# DSperate experiment — Platinum first

This branch embeds DSperate in the Linux executable behind `POCKET_DSPERATE`,
which defaults OFF. It does not replace mGBA, the Android melonDS backend or
upstream's normal Linux build. It is not an upstream-ready DS support claim.
No Pokémon ROM, save, proprietary BIOS or game art is bundled.

## Implemented, with limited validation

- `.nds` launch, ARM9/ARM7 emulation, audio, one DS screen at a time, screen swap,
  physical controls and a movable stylus cursor.
- Separate battery-save and DSperate-state paths, startup backup of an existing
  battery save, atomic disk replacement and a bounded wait for writes to settle.
- An experimental **read-only, text-based companion** for the exact Platinum
  USA Rev 1 ROM recognized by the Android app:
  SHA-1 `0862ec35b24de5c7e2dcb88c9eea0873110d755c`.
  It shows party, HP, held items, moves/PP, bag, trainer, money and location.
  Summary Left/Right switches between moves and information (EXP, nature,
  gender, egg flag and health status); Up/Down switches party members.
  DEX reads upstream seen/caught flags, counts and National unlock status. It
  browses species by National number; unseen names are hidden. Left/Right pages
  through the dex or bag. This does not implement Sinnoh ordering or dex entries.
  MAP decodes the Town Map background, town blocks and player heads from the
  owner's ROM. It uses the generated grid, live location/indoor exit fallback
  and upstream first-arrival flags. D-pad moves across visible grid cells
  (including blank ones); A recenters. No Fly action or game-memory writes.
  This is not the full Android companion; there is no field action,
  sprites or battle UI replacement yet. SET persists DS screen, expansion and
  optional auto-resume preferences independently of GBA settings.
- Experimental 340×256 overworld view: optional DSperate hooks capture geometry
  before clipping and render eight auxiliary views. The original 256×192 center
  interior is copied from the real screen, not stretched. The one-pixel
  viewport perimeter is reconstructed from overlapping geometry where the
  original 3D output matches the displayed pixel, preserving 2D overlays. Upstream's Platinum RAM field
  state gates expansion off during battles, menus and locked field tasks.
  Margins use DSperate's RGB conversion, master brightness and BG0 brightness
  fades. Alpha blending against other 2D layers, scanline-specific effects,
  text overlays and transitions need further testing. Auxiliary views overlap
  the original viewport so their raster borders are outside the joins.
  Auxiliary rendering may be too expensive on a Brick; it is OFF initially.

The native tests use a captured upstream Platinum RAM fixture, an original
logo-free diagnostic cart and actual DSperate polygon rendering. A synthetic
quad test is evidence of the rendering hook, **not** evidence that Platinum's
world expands correctly. Commercial-game boot, save integrity through real
saving, field detection, expansion appearance, audio and hardware performance
must be tested with the owner's ROM and a disposable copy of their save.

## Build

Use the normal dependencies in `linux/README.md`, plus a C++17 compiler.

```sh
git submodule update --init third_party/mgba third_party/dsperate
python3 linux/dsperate/gen_data.py --check
cmake -S linux -B linux/build-ds -DCMAKE_BUILD_TYPE=Release -DBUILD_LTO=OFF -DPOCKET_DSPERATE=ON
cmake --build linux/build-ds -j2
ctest --test-dir linux/build-ds --output-on-failure
linux/build-ds/pokedaisy-linux --data /your/disposable/ds-test /your/Platinum.nds
```

With CMake 4, older mGBA also needs `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.
TrimUI uses the same pinned SDK and toolchain as the normal port:

```sh
python3 linux/setup_trimui_sdk.py /absolute/path/trimui-sdk
export NEXTUI_TOOLCHAIN=/absolute/path/trimui-sdk/cross/x86_64-aarch64-nextui-linux-gnu
cmake -S linux -B linux/build-ds-arm -DCMAKE_TOOLCHAIN_FILE=trimui-toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_LTO=OFF -DPOCKET_DSPERATE=ON
cmake --build linux/build-ds-arm -j2
python3 linux/package.py --dsperate linux/build-ds-arm/pokedaisy-linux /new/package-output
```

The package flag includes DSperate/FreeBIOS/miniz/font notices and labels the
archive as an experiment. Install in its own app/Tools folder and keep the
working GBA package. Do not replace system libraries. ARM builds enable
DSperate's JIT and NEON, but QEMU results cannot establish hardware frame rate.
DSperate's unconditional LTO is disabled for its library because the pinned
TrimUI GCC 8.3 package lacks `lto1`; GCC 8 also requires `stdc++fs`.

## Controls

| Action | Handheld | Keyboard |
| --- | --- | --- |
| DS D-pad, A/B/X/Y, L/R | Corresponding controls | Arrows, Z/X/S/A, Q/E |
| DS Start / Select | Start / Select | Enter / Backspace |
| Companion / game | L2 | Tab |
| Swap DS top/bottom screen | R2 | Space |
| Aim on bottom screen | Hold Select + D-pad | Hold Backspace + arrows |
| Touch at cursor | Hold Select + A | Hold Backspace + Z |
| Pause | Menu | Escape |
| Resume / save state / load state / quit, while paused | B / A / X / Start | X / Z / S / Enter |
| Toggle expansion, while paused | Y | A |
| Toggle expansion during play | — | F |
| Companion tabs / selection / summary / back | L/R, D-pad, A, B | Q/E, arrows, Z, X |

The stylus has a crosshair while Select is held on the bottom screen. These
bindings are separate from the GBA frontend; controller hot-plug and complete
shortcut customization are not implemented. Use a pad connected before launch.

## Save paths and state behavior

Given `Platinum.nds`, use `DATA/saves/Platinum.nds.sav` for a raw battery-save
copy; complete ROM filenames intentionally separate DS from other backends.
Wrong-size input is rejected. There is no automatic save import or migration.
State files are `DATA/states/Platinum.nds.dsperate.ss`. They are DSperate-specific,
include cartridge battery contents, and are not compatible with melonDS or
DraStic states. Loading an older state can revert battery contents when the
emulator subsequently saves. Never test against the only copy of a save.

Use the game's Save menu first, wait for it to finish, then quit via Menu →
Start. A forced shutdown can lose changes since the last disk flush.
A failed state load exits without flushing a potentially half-restored battery.

## Data and renderer boundaries

`gen_data.py` derives the reader's configuration, shuffle orders, character
mapping, names and identity from the existing Kotlin sources. `data_gen.h` is a
checked generated output, not a separately maintained game-data table. Run the
generator after changes upstream. Only printable ASCII nickname mappings are
rendered by this experimental font path.

The adapter builds the RAM-only prefix of upstream `pk_nds_field.cpp`; its
melonDS GPU-only helper is excluded at configure time. CMake fails if that
boundary changes. The original source and all shared C core files are unchanged.
This is a temporary adapter, not a proposal for Lidor's data architecture.

`third_party/dsperate` pins DSperate to
`e04b806b5c01527b08a8e7181476ada57e97db52`. The small geometry patch lives in
`third_party/patches/dsperate-pokedaisy.patch`, applied idempotently by CMake.
After configuring, Git therefore reports the submodule as modified; the patch
is the maintained source of those changes. No DSperate history is rewritten.

For reproducible frontend smoke tests, `POCKET_DS_KEYS` accepts comma-separated
`frame:DS-button-bitmask:duration` entries and `POCKET_DS_COMPANION_AT=N` pauses
into the companion at host iteration N. Normal `--frames` / `--shot` also work.
These are test controls, not evidence of a live Pokémon run.

## Companion compatibility audit

| Upstream capability | DS Linux experiment |
| --- | --- |
| Party / basic summary / moves | Live read-only data; limited text presentation |
| Bag | Live names/quantities; scrolling and page movement |
| Pokédex | Live seen/caught flags and counts, National-number browser |
| Regional ordering, dex descriptions, pictures | Not integrated; upstream ROM-art extraction needs a Linux path |
| Trainer/location | Basic live fields and National Dex unlock status |
| Town map / visited destinations | ROM-derived map, grid navigation, player marker and upstream arrival flags; read-only |
| Battle telemetry / enemy party | Upstream decoder exists; not integrated |
| Repels / field actions | Upstream RAM/call paths exist; not integrated or tested here |
| DS persistent settings / auto-resume | SET tab; separate clean-exit resume slot, opt-in |
| Other supported DS ROMs | Companion disabled until a matching adapter is verified |

The current shared C companion targets Gen 3. Reusing its widgets is practical,
but passing DS data through its game-specific readers is not. The experiment
keeps DS reads separate and generates reference data from the Kotlin sources.
A shared game-independent snapshot boundary should be agreed with upstream
before attempting full menu parity. No parallel game-address registry is added.

## DS settings and auto-resume

Open the companion with L2, then use L/R to select SET. Up/Down selects SCREEN,
EXPANSION or AUTO-RESUME; A toggles it. Preferences are atomically saved to
`DATA/ds-settings.ini`, separate from the existing GBA settings file. Screen
swaps (R2) and expansion changes (F or paused Y) also update DS preferences.
Expansion and auto-resume both default OFF. These settings apply to DS games
in this data directory, while resume files are per ROM filename.

When AUTO-RESUME is ON, a successful clean exit captures
`DATA/states/ROM.nds.dsperate.resume.ss`. The next launch restores it; manual
states remain in the separate `.dsperate.ss` slot. A forced shutdown does not
capture a new resume state. Keep using the game's own Save menu too.

Resume states contain battery memory. A newer battery-save timestamp skips an
older resume file, allowing a newly supplied save to boot normally. Invalid,
truncated, wrong-ROM or incompatible resume states fall back to a fresh boot;
restoration occurs in a separate console so a failed load cannot partially
corrupt the fresh session. The next successful clean exit replaces that resume
file. Filesystem timestamps are a precedence hint, not a substitute for backups.

`POCKET_DS_RESUME=0` bypasses auto-resume and automatic resume capture for one
launch, leaving an existing resume file untouched. `POCKET_DS_EXPAND=0` similarly
overrides the initial expansion preference. Changing a preference in the UI
persists the current DS preferences. No GBA save, state or setting is migrated.

## Platinum Town Map

L/R selects MAP. The D-pad moves one grid cell at a time and repeats while
held; blank cells display "No location". A returns the cursor to the player's
block. The cursor is constrained to the displayed map area. The player's head
stays at their own block while browsing elsewhere.

The background's two tilemaps, town-marker cells and player heads are decoded
at launch directly from `/graphic/tmap_gra.narc`, following Android's NdsArt
pipeline. Nothing is exported into or bundled with the application package.
Grid data, section names, pointer/field offsets, sprite member ids and map
coordinates are generated from existing upstream sources. No independent map
or address list is maintained.

Live field position is used outdoors; when indoors the saved exit block is
used, matching upstream's Town Map reader. Visited colouring follows the same
56 first-arrival bits as that reader; special island markers beyond that bit
range remain limited by the upstream telemetry. Fly selection, island unlock
extensions and route previews are not implemented. Only Platinum USA Rev 1
is enabled, and physical Brick navigation still needs validation.
