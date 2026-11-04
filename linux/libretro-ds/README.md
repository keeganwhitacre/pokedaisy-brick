# PokeDaisy DS libretro — first Brick test build

Experimental Platinum milestone, 2026-11-04. This is a separate core and pak.
It does not replace the working mGBA PokeDaisy core, the NDS pak or brick/dev.
Physical Brick validation has NOT been performed by this development session.

## Install on NXRedux

1. Extract the package at the SD card root. It adds only `Emus/tg5040/PDDS.pak`.
2. Put a COPY of your Platinum ROM in `Roms/PokeDaisy DS (PDDS)/`.
3. Launch from NXRedux. Use Aspect scaling, with no crop/overscan. The normal
   256x192 gameplay frame fits the 1024x768 Brick at exactly 4x. No panel option
   is required. The companion uses 341x256 (3x, one spare horizontal pixel).
4. For the first test, keep saves isolated in the new PDDS system. A supported
   retail Platinum USA revision 1 is required for the companion and game-specific
   automatic screen rules. Other ROMs can boot but have no verified companion.

| Control | Action |
| --- | --- |
| D-pad, A/B/X/Y, L1/R1, Start | Native DS controls |
| L2 | Companion open/close; emulation pauses while open |
| R2 | Cycle automatic / bottom only / top only |
| Select + D-pad | Move stylus (automatically exposes bottom screen) |
| Select + A | Hold stylus down; release A to lift |
| Select tap | Forward DS Select on release |
| Companion L1/R1 | Party / Bag / Dex / Map |
| Companion A/B | Details / back |
| Native MENU / shortcuts | Frontend menus, states, fast-forward, resume |

For drawing, keep Select+A held while moving the D-pad. Pen coordinates and
pointer reprojection are implemented; actual Brick interaction still needs a
hand test. Set shoulder shortcuts so L2/R2 reach the core; this pak's default
bindings expose both. R2's manual override persists until cycled back to AUTO
or a state is loaded/reset. All four DS face buttons remain available.

The party HUD is an optional core option (`pokedaisy_ds_hud=enabled`), off by
 default. It uses the existing shared party HUD and DS ROM icons. It appears
only on the top-only layout and can cover game content. The separate GBA/GB
core's quick-menu field actions are not yet ported to this core.

## Saves, states and resume

Minarch owns SRAM writes, save slots, quick resume and shortcuts. The core does
not create a competing save directory or write a second Platinum save file.
NXRedux uses `Saves/PDDS` for this separate tag, and its own user-data area for
states. Its selected save-format setting determines the SRAM filename/extension.
A raw Platinum save chip is 524288 bytes; the supplied raw save loaded in tests.

To import, make a backup, select an uncompressed SRAM format in the frontend,
let this new pak create its save filename, quit normally, and replace only that
new PDDS file with a copy of your raw save. Do not copy over the original NDS
save. A DraStic `.dsv` with a footer requires conversion to raw SRAM; it is not
validated here. Old SDL-host DSperate states used a different DSperate revision
and container: do not rename them as minarch states. Raw battery-save import is
the migration path tested here. Emulator states are version-dependent.

The pinned dsperate-lr and enhanced core accept each other's state containers;
that does not make states compatible with melonDS, DraStic or future DSperate
versions. A successful state load closes stale companion UI and refreshes its
snapshot. An invalid machine state follows DSperate's reset-on-failure policy.

## What this milestone implements

- DSperate v3.3.0 through the unmodified dsperate-lr v3.3.0-lr.1 wrapper.
- Separate `pokedaisy_ds_libretro.so`, no SDL launcher or mGBA dependency.
- Existing Platinum party/summary, bag/details, Dex/filter and map views from
  `linux/dsperate`, using the same `3ds/core` canvas, UI and generated app data.
- The current upstream RAM-only `pk_nds_field` reader, including Platinum's
  party/options top-screen rules and battle touch-panel detection.
- Top-only gameplay, bottom-only for battle selection/touch-only screens,
  side-by-side for other touch menus, optional stacked layout, manual fallback.
- Upstream's one-colour fallback for bottom-only boot questions, and retaining
  the current layout through fully blank scene transitions.
- RGB565 software output and dynamic libretro geometry; native frontend
  postprocessing remains available. Actual shader behavior needs Brick testing.

This is not Android 1.4 parity. Android's floating/animated DS layout, complete
fade hysteresis, custom battle-button overlays and all field actions are not
ported. Native game battle input is forwarded, with the stylus fallback; no
live battle was validated in this session. Automatic rules are tested against
synthetic menu states using app-owned metadata and a captured RAM fixture;
full battle/menu transition playtesting remains outstanding.

## Build

Initialize only the new dependency tree (the old DS baseline remains pinned):

```sh
git submodule update --init --recursive third_party/dsperate-lr
cmake -S linux/libretro-ds -B build/ds -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/ds -j4
ctest --test-dir build/ds --output-on-failure
```

For the Brick, use the established TrimUI SDK:

```sh
python3 linux/setup_trimui_sdk.py /absolute/sdk-directory
export NEXTUI_TOOLCHAIN=/absolute/sdk-directory/cross/x86_64-aarch64-nextui-linux-gnu
cmake -S linux/libretro-ds -B build/ds-arm -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/linux/trimui-toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build/ds-arm -j4
python3 linux/libretro-ds/package.py build/ds-arm/pokedaisy_ds_libretro.so \
  PokeDaisy-DS-NXRedux-Test.zip --strip "$NEXTUI_TOOLCHAIN/bin/aarch64-nextui-linux-gnu-strip"
```

Keep NEXTUI_TOOLCHAIN set during incremental builds, since the existing
 toolchain file reads it on reconfiguration. This SDK uses GCC 8.3, no LTO or
release PGO. ARM builds enable DSperate JIT/NEON; x86 builds use the interpreter.
The C++ runtime is linked statically; the ARM test binary needs only libc,
libm, libdl and libpthread, with GLIBC <= 2.18. It is not the GCC 13.3/PGO build
from Thwonp's Docker workflow. Performance must be measured on the Brick.

## First physical test

- Launch the copied Platinum and continue into the field; check all screen edges.
- Open every companion tab, party summary, bag details and Dex filter; return.
- Open native party, bag/options and a wild battle; verify automatic layouts.
- Try R2 fallback and Select+D-pad/A tapping/dragging. Verify all face buttons.
- Make an in-game save, quit/relaunch, then save/load a native state.
- Test frontend quick resume, sleep/wake, fast-forward and a shader separately.
- Listen for audio underruns during movement and a battle; report the scene.

The current NXRedux minarch has no audio-buffer-status callback. Therefore
DSperate's automatic/slotplus frameskip is disabled by its own fallback. This
pak defaults to disabled; an auto option cannot create the missing frontend
hint. No physical speed, audio, battery-life or sleep/resume claim is made.

See AUDIT.md for source pins, architectural findings and validation details.
