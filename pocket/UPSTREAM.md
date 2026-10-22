# Maintaining the native port

This is a branch of lidor30/pokedaisy with its original history, not an independent
copy of the application. The current upstream base is recorded in the release's
Verification/build.json. Keep the original LICENSE and NOTICE.

## Boundaries

- `main.c`: SDL, controller events, emulator memory adapter, audio/video, save and
  lifecycle management. It translates buttons to companion actions.
- `polished.h`: the Polished companion contract: identity, memory snapshot,
  navigation state and drawing. No SDL or mGBA types leak into it.
- `polished.c`: ROM identity, party/bag codecs, original mini-icon art and layouts.
- `polished_battle.inc`: battle INFO/foe/team layout and the exact SuggestionsScreen ranking heuristic; optional live fields decoded from upstream Gen2Reader. `pc_ui_observe` tracks announced foes each emulated frame and resets between battles.
- `polished_guide.inc`: GuideWorld/GuideScreen/GuideAreas adaptations. Guide source data is regenerated through `gen_polished_guide.py`, not edited in C tables.
- `polished_parity.inc`: original ROM town-map/player extraction and the adapted Android summary.
- `polished_sections.inc`: Gen2Reader/Gen2Dex adaptations: battle, dex, ROM text,
  front art, item descriptions and game-specific navigation. Included in the same
  translation unit to reuse bounded ROM codecs without exposing their internals.
- `gen_polished.py`: generates species, moves, types, items, forms, build-specific
  ROM tables, character/font maps, dex order, landmarks, type effectiveness and RAM addresses from the Android source.
  `upstream-tables.json` records SHA-256 hashes of every source input. Generated
  data is committed; Python is a development dependency, not a device dependency.
- `3ds/core/`: upstream's reusable GBA companion. Keep Brick behavior in pocket/
  whenever practical rather than changing the portable UI for the handheld.

This reduces manual duplication; it does not translate Kotlin UI or algorithms
into C automatically. A changed struct layout, compressed format or new feature
still needs an explicit adaptation and tests.

## Updating

The port lives at https://github.com/keeganwhitacre/pokedaisy-brick. Keep origin on that repository and upstream on the original:

```
git remote set-url origin https://github.com/keeganwhitacre/pokedaisy-brick.git
git remote add upstream https://github.com/lidor30/pokedaisy.git
# If upstream already exists, leave it in place.
git fetch upstream main
git switch main
git merge upstream/main
python3 pocket/gen_polished.py
python3 pocket/gen_polished.py --check
git diff -- pocket/polished_tables.h pocket/upstream-tables.json
```

Review the generated changes alongside their Kotlin sources. The generator fails
on missing RAM fields, character-map or ROM-table schema changes instead of
silently retaining the old definitions. Hash changes alone do not establish
compatibility. Check changed decoder logic in Gen2Reader.kt and Gen2Dex.kt too.

Build with POCKET_TESTS=ON, run decoder and runtime tests in VERIFICATION.md, then
cross-build with the NextUI toolchain. Test a matching personal ROM on Brick,
including save/reload, before declaring a new build hardware verified.

The Pocket native workflow provides the generation/build/test checks on GitHub;
it now also defines an ARM cross-build/QEMU job. See Actions for remote run results; local tests do not imply remote CI success. The existing Android workflows
remain upstream-owned and may require setup/secrets in a fork.

## Adding a screen or game

For another Polished screen, extract existing upstream definitions in the
generator, adapt its read algorithm behind pc_read, and add a section/action to
pc_ui_input/pc_ui_draw. Read paused emulator memory through pd_read_fn; never
introduce test data or write game state in production to make a screen work.

For another game, reuse the host and portable canvas, and implement the same
separation between identification, read-only snapshot, controller actions and
rendering. Exact ROM-hack identification must happen before retail header
fallbacks: a shared game code does not mean shared memory structures.

## Fork status

The current GitHub connector has no create-repository or fork operation. Source
and commits are complete locally, but no fork or remote push is claimed. Create a
fork named pokedaisy-pocket at https://github.com/lidor30/pokedaisy/fork; then the
handheld branch can be pushed with its full upstream ancestry and no ROMs/saves.

The SDK setup script pins both archive SHA-256 values and confines extraction to its destination. It preserves GCC linker scripts when overlaying target SDK headers/libraries. Neither the SDK nor private ROMs/saves belong in git.
