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
  This is not the full Android companion; there is no dex, map, field action,
  sprites, battle UI replacement, auto-resume or persistent DS settings yet.
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
