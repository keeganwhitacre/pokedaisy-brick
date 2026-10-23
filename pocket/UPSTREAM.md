# Maintaining the native port

This is a branch of lidor30/pokedaisy with its original history, not an independent
copy of the application. The current upstream base is recorded in the release's
Verification/build.json. Keep the original LICENSE and NOTICE.

## Boundaries

- `quick_menu.inc` and `gen_quick.py`: native quick-menu overlay and driver button discovery; original Android pixel art is generated into quick_icons.h with recorded source hashes.
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

The fork is https://github.com/keeganwhitacre/pokedaisy-brick, branch `main`, retaining upstream ancestry. Keep `lidor30/pokedaisy` as `upstream`; merge and resolve the small native adapter in `pk_wide_gb.c` when that shared renderer changes. Generated Polished definitions remain version-pinned; companion compatibility does not automatically grant expansion compatibility.

The SDK setup script pins both archive SHA-256 values and confines extraction to its destination. It preserves GCC linker scripts when overlaying target SDK headers/libraries. Neither the SDK nor private ROMs/saves belong in git.

## Phase 10 synchronization and boundaries

Upstream merged through `54d8f4867501feb32b7ee139a5616b7c6301f044` with both parents retained. All source changes are preserved for later game ports. The GB renderer's native adapter remains separate; no upstream Polished address changes were required. `gen_quick.py` refreshed its input hashes against the new FieldHud source; extracted icon pixels are unchanged.

- `app/src/main/cpp/pk_savevf.c/.h`: upstream implementation, byte-for-byte unchanged. Keep it shared; do not copy it into a game decoder.
- `pocket/host_io.c/.h`: Linux host policy around that VFile (error latch, write inhibition after failed reads, ownership tracking), plus per-content display preferences. No Pokemon memory definitions or ROM art.
- `pocket/rom_identity.h`: the existing verified Pocket SHA-1 implementation moved out of Polished and reused for host identity. Changing a filename does not change preferences.
- `pc_ui`/Guide: controller adaptation of upstream GuideUiState single-open-entry, hint, answer and close behavior. No emulator calls enter the companion renderer.

New games should reuse host I/O, identity, controller events and emulator lifecycle. Add game-specific memory definitions behind snapshots and reuse the upstream game implementation/assets; do not add ROM-specific save/display branches to main.c.

## Phase 11 reusable experience and animation

`experience.c/.h` is a portable, game-independent adaptation of SnapshotView ExpProgress, with all six upstream ExpTables curves. `gen_experience.py --check` validates table shape and records source hashes. Each game maps its own growth-rate ids before calling this module; Polished's mapping and species rates come from upstream Gen2SpeciesPolishedCrystal/SnapshotView via gen_polished.py.

`gen_party.py` extracts PartyScreen's HP lettering with source provenance. The native party layout follows SlotGeometry/drawSlotFrame/drawHpBar using the existing portable canvas rounded rectangles; Polished's transparent ball palette is respected. `pc_ui_tick` accepts a caller-supplied monotonic UI clock: companion animation never advances the emulator or depends on SDL in the renderer. `partyActive` is optional snapshot telemetry, initialized to -1 by readers; the Polished adapter fills it from upstream curBattleMon only in a validated battle. Generic host code contains no new Pokemon addresses.

## Phase 12 shared drawing widgets

`pixel_widgets.h` adapts PartyScreen.drawHpBar, BattleLists.drawListHpBar and MapScreen.drawMapCursor as portable canvas operations. HP colors come from a caller-supplied palette; no game ids, offsets, ROM files, SDL calls or emulator timing enter the helpers. Polished supplies its upstream palette. Party and battle share the same health renderer, while `pc_ui_tick` supplies the map's 20-frame/333 ms cadence. Map cursor geometry follows the selected landmark dimensions and leaves the player visible. Future game adapters can reuse these widgets with their own verified palettes.

The item/next-foe/animation parity audit is recorded in PARITY.md. Keep Gen 2 source boundaries separate from richer GBA/DS paths rather than supplying unsupported telemetry or assets.

## Phase 13 repeatable visual parity

`gen_style.py` records/extracts the PolishedCrystalBag/YellowBag frame layers, backdrop/underline palette, GbaMenu's Game Boy OPTION frame, and PixelIcons/PokedexScreen seen/caught marks. It records source hashes and fails on unexpected frame/palette shapes. `--check` is part of native CI. No ROM graphics are added to the source or package.

`item_list.h` is a game-independent, stable O(n log n) view sort with caller-owned scratch. It orders Default/Name/Type/Count as ItemsScreen does and preserves duplicate stacks. Polished's memory-to-item adapter remains in polished_bag.inc; the SDL host has no new game-specific branches. Shared pixel_widgets provides clipped opacity and full-description pagination without SDL, allocations or emulation calls.

The upstream `ui-preview` harness now also rebuilds Gen 2 ROM art and renders Polished captures with `-Pgen2Fixture=polished_crystal`, using readGen2Telemetry/buildSnapshotView and the exact upstream hash/faithful/table selection. In-game window scope is restored when each companion preview closes. See ui-preview/README.md. This provides source-rendered reference captures for future screens and avoids interpreting generic preview fallbacks as the Android game's actual style.

## Phase 14: Guide translation and portable text

The Android UI remains unchanged. The native Guide adapts GuideScreen's
page/section/entry hierarchy and single open-key model on the existing canvas.
`gen_style.py` now extracts Guide page labels and method order directly and
tracks GuideScreen's hash. GuideWorld tables continue to come from the existing
strict data generator. No new ROM offsets or game actions are introduced.

The shared canvas gains explicit newlines; the existing font generator adds
the middle dot from upstream's bundled Pixel Operator, used as a fallback when
the cart font lacks it. This is reusable by future Linux game adapters.
A larger shared Compose/C UI schema could reduce duplication but would change
the original rendering architecture and introduce a new synchronization
surface. This pass intentionally keeps that deferred: small source-derived
definitions, shared pixel primitives and matching-source captures provide an
incremental contribution boundary without replacing the working emulator host.

User hardware confirmation now covers gameplay, expansion/zoom, companion,
saves and quick menu. It does not imply the new Guide layout, sustained
performance, RTC or power handling have been verified on the device.

## Phase 15: portable action and display boundaries

`game_action.c/.h` is a read-only observer/controller-bit state machine; SDL
owns pacing, input exclusion, cancellation and reporting. Only the verified
normal 3.2.3 observation adapter is enabled. This first Bag opener is downstream,
while Polished battle controls/switching are existing Android functionality.
See ACTIONS.md for official game source/symbol provenance; do not propagate
its offsets into generic menus or widen the ROM allowlist without verification.

`display_fx.c/.h` translates ScreenShaders LCD, SCANLINES and GBA_COLOR math.
It has no emulator/SDL dependence. The host caches one alpha texture per output
geometry/filter/grid and preserves the existing opaque source textures.
GRID defaults to MEDIUM as in Prefs. Defaults remain filter NONE, colors OFF,
original integer aspect. The new *.fx file does not replace *.mode or INI files.

On an upstream update review ScreenShaders/EmulatorView/Prefs changes together,
including prescale/smoothing/companion semantics. Do not claim unsupported
passes through similar-looking software effects. The combined SCREEN / SHADERS
host menu remains a documented controller adaptation. The existing Compose
preview cannot validate EmulatorView's GL pipeline; native reference-equation
checks are explicitly narrower than Android GL screenshot parity.

## Phase 16: lifecycle and session boundaries

WideCamera.frame / Prefs.expandZoomOnText supply the dialogue policy. The shared
renderer remains unchanged; only ready FIELD and opted-in TEXT frames expand.
AUTO ZOOM IN defaults OFF by request here, versus ON on Android. Do not widen
the readiness predicate to conceal unsupported menus, fades or map matches.
Exact Android camera timing remains separate unfinished work.

`platform_nextui` owns foreground Linux power events, helper execution and the
launcher shutdown marker. `recovery` owns the bounded serialization envelope,
SRAM-content check and atomic replacement independently of SDL/game decoding.
The existing frontend coordinates core/audio/input/textures between frames.
LIFECYCLE.md records the actual NextUI source commit and audited functions.
On updates recheck power ownership, suspend helper/launcher contracts, mGBA
state compatibility, GB/GBA SRAM access and absence of savedata in recovery.

The optional Emus wrapper reuses the existing Tools executable/data root.
Android source, shared expansion implementation, save wrapper, ROM allowlist
and game memory offsets are untouched. No portable renderer/schema rewrite
is needed for these platform features. Polished's unspawned/connected objects
remain an explicit game-adapter gap, rather than generic SDL policy.

## Phase 17: original battle interactions

`battle_action` translates the Android Gen 2 menu readers and closed-loop
controller: read-only callbacks in, normal key bits out. It keeps game addresses
and party identities out of SDL and never writes a cursor. The host owns input
exclusion, cancellation (including power/system hotkeys), real-frame pacing and
return to the single-screen companion. Existing game_action remains the
independent Bag opener. INTERACTIONS.md records source hashes and retry rules.

UI action/move cells adapt Gen1ActionButtons/Gen1MoveList (also used by Gen 2);
the existing party grid adapts PartyPicker. Information pages remain intact.
The Brick pauses while choosing and shows actual game execution between
choices. Processing-ball/fades and held-frame switching remain differences.
Follow-up should validate genuine multi-mon switches before widening the
tested game/build scope.
