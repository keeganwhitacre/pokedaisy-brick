# One native PokéDaisy collection — GB, GBC, GBA and DS

This integration packages two cores behind one NXRedux emulator entry:

- `pokedaisy_libretro.so`: Thwonp's GB/GBC/GBA implementation, source pinned at
  4ab40fac9c7255853c9c32c1de227836e6f699e2 on feat/libretro-core.
- `pokedaisy_ds_libretro.so`: the Platinum DSperate implementation with the shared
  companion/settings and the new DS quick menu.

This is an integration branch in keeganwhitacre/pokedaisy-brick:
`brick/libretro-unified`. It does not merge or push changes into Thwonp's repo.
His GB/GBA source is pinned as a build dependency, not rewritten. The cores
currently build against their respective portable UI versions. Consolidating
those versions into one source tree still requires porting the DS presentation
adapters onto his newer party/HUD APIs; this package does not claim that merge.

## Install

Extract the zip at the SD root. It adds `Emus/tg5040/PDAI.pak` and the collection
`Roms/PokeDaisy (PDAI)/`, with optional Game Boy / Game Boy Color / Game Boy Advance /
Nintendo DS subfolders. Put your own `.gb`, `.gbc`, `.gba` or `.nds` files in this
collection. The extension selects the core automatically. Case and spaces work.
Extract archives first; ZIP dispatch is deliberately not guessed.

GB/GBC/GBA games use Thwonp's existing companion, HUD, quick menu, settings,
EXPAND and ZOOM OUT on the games he supports. The Brick panel is preset to
1024x768. DS retains full-size 256x192 gameplay (4x on the Brick), auto/manual
screen layouts and the Platinum companion. There is no SDL launcher.

The old DS pak and your existing GB/GBA installation are preserved. This is a
new frontend tag, so importing previous progress requires COPIES of saves to the
new frontend's expected filenames. Keep the originals as a rollback. Prefer
raw battery saves for migration; native state paths include the core and tag.
No automatic save conversion or copying occurs.

NXRedux derives its emulator/save tag from the TOP-LEVEL collection folder,
not nested folders: all games here use PDAI. Use distinct ROM basenames across
systems (e.g. `GBC - Crystal.gbc`, `GBA - Emerald.gba`, `DS - Platinum.nds`). Its
SRM formats remove the ROM extension, so identically named games can otherwise
share an SRAM filename. Nested folders do not solve that frontend constraint.

GB/GBA preferences remain in `<save directory>/pokedaisy/settings.ini`; DS uses
`<save directory>/pokedaisy-ds/settings.ini`. DS only imports a legacy file if
it contains DS-specific metadata, and never overwrites the old file. Thus the
two engines cannot overwrite each other's preference keys. NXRedux still owns
SRAM writes, save states, quick resume, shortcuts, scaling and shaders.

## Controls and parity

| Feature | GB/GBC/GBA | DS / Platinum |
| --- | --- | --- |
| Native NXRedux launch and state API | Thwonp core | DSperate core |
| Companion/settings | Thwonp UI | Shared portable frame and existing Platinum adapters |
| Quick menu | Thwonp's existing actions | Select + companion button: Party, Bag, Map, Dex, Settings, HUD |
| Quick-menu layout | Thwonp settings | Companion Settings > QUICK MENU: RING / GRID |
| Gameplay screen | Thwonp aspect/panel logic | R2 manual swap by default; automatic layouts |
| EXPAND | Supported game-specific implementations | Not shipped |
| ZOOM OUT | Existing implementation | Separate research proof only |
| PC / Fly / Bike / Repel / Wild field actions | Existing supported-game actions | Not yet ported |
| Advanced HUD layouts/look/cursor | Thwonp implementation | Existing party HUD only |

DS defaults: L2 companion, R2 screen selection, Select+D-pad/A stylus. Hold
Select and press L2 for the quick menu (or Select + your remapped companion
button). D-pad chooses, A applies, B cancels. The menu pauses DS emulation and
blocks game input. Settings > QUICK MENU changes Ring/Grid persistently. X/Y
remain native DS buttons unless you explicitly assign an overlay to them.

On GB/GBA, Thwonp defaults X to companion and Y to quick menu. His own companion
Settings can change these. There is no fake DS EXPAND switch or inert PC/Fly tile.

## Reproduce the build

```sh
git submodule update --init --recursive third_party/dsperate-lr
git submodule update --init third_party/pokedaisy-libretro
git -C third_party/pokedaisy-libretro submodule update --init third_party/mgba
# Set NEXTUI_TOOLCHAIN as documented by linux/libretro-ds/README.md.
cmake -S third_party/pokedaisy-libretro/libretro -B build/gb -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/linux/trimui-toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build/gb --target pokedaisy_libretro -j4
cmake -S linux/libretro-ds -B build/ds -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/linux/trimui-toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build/ds -j4
python3 linux/libretro-unified/package.py build/gb/pokedaisy_libretro.so \
  build/ds/pokedaisy_ds_libretro.so PokeDaisy-Unified-NXRedux-Test.zip \
  --strip "$NEXTUI_TOOLCHAIN/bin/aarch64-nextui-linux-gnu-strip" --source-ref YOUR_COMMIT
```

The SDK build uses GCC 8.3 without DSperate release PGO/LTO. Actual combined-pak
launching, performance/audio and resume still need a fresh physical Brick test.
The prior standalone DS and GB/GBA builds worked on the owner's device.

## DS zoom-out research

`projection_probe.py BUILD OUTPUT` copies one DSperate geometry source and
archive, injects a private clip-coordinate scale, and links a separate probe
core. It uses Ninja's compile/link commands; normal sources, archives and cores
are not modified. Run with `PD_DS_PROBE_PERCENT=100`, `90`, or `80` and copied
saves/states. The field screenshots demonstrate more Jubilife scenery at 80%
using a single geometry/raster pass. This is not wide rendering at extra pixel
resolution: the target remains 256x192, and the world becomes smaller.

The probe scales ALL submitted 3D, including possible 3D UI. It does not yet
handle game-side culling, scene gating, stale geometry on setting changes or
presentation independence across save states. It is not packaged as the normal
DS core and no supported EXPAND/ZOOM OUT option is claimed. Those issues must be
resolved before exposing it as a persistent setting or asserting Android parity.
