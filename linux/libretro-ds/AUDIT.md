# DS libretro audit and validation — 2026-11-04

## Inspected revisions

| Source | Revision / scope |
| --- | --- |
| keeganwhitacre/pokedaisy-brick brick/dev | ec441a754f8de4cd231dbc126ef9ac65e2bd5992, preserved |
| brick/ds-baseline | 3be86d73ffb1c57c239e0b67d46e979deaf04454, development base, preserved |
| lidor30/pokedaisy main | 54f4c66f5ef0857192838d877418c7db96ccfa99 |
| thwonp/pokedaisy-plorp v0.3-pre2 | 3d4a69e9880f27bfccf2d06a46e9743f87883ed4 |
| thwonp/dsperate-lr v3.3.0-lr.1 | 801a2dc (tag object f78870a; HEAD later changes README only) |
| DSperate pinned by wrapper | 99618eb956c66083618395dbae9bb5cbf0241d16, v3.3.0 |
| NXRedux nx-handheld | 1168e77949e5c686d169e742157638b24c12aa66 |

Upstream PR #324 is merged (0620bcaf41644c37542e25abc8b80e3cfa510b3e).
The older baseline report's statement that it was open is historical.
No existing remote branches were moved and no code was pushed to either
Lidor's or Thwonp's repository.

## Architecture

Thwonp's GB/GBA core includes mGBA's wrapper under renamed entry points, then
layers shared PokeDaisy UI/input/rendering over its lifecycle. Its memory adapter
is explicitly mGBA/GB/GBA-specific; substituting a DS framebuffer would not add
DS memory semantics. We use the same composition approach around dsperate-lr:
`pd_ds_libretro.cpp` includes the pinned wrapper under renamed entry points and
intercepts only lifecycle/UI boundaries. Its memory, state, audio, JIT and
SRAM paths remain DSperate's. The external ABI exports only `retro_*`.

The new target lives under linux, alongside the existing Linux SDL host and
3DS host, and compiles shared portable `3ds/core` sources. It links the existing
Platinum reader and views from linux/dsperate; there is no second party/bag/Dex
reader or new hand-maintained game table. `data_gen.h` is unchanged and still
comes from app-owned Kotlin data. ROM hash guarding prevents use of revision-
specific offsets/art on another cart. Game RAM is read through DSperate main
RAM and its ARM9 bus; cartridge bytes feed the existing NARC art decoders.

The old third_party/dsperate pin is untouched. A separate nested
third_party/dsperate-lr dependency pins the wrapper and its matching emulator.
This temporarily retains two source dependencies, so the SDL baseline stays
reproducible. Neither emulator is duplicated in the new binary.

The fork imports upstream's current RAM-only field reader and separate
pk_nds_field_gpu.cpp, updating Android/nds_dump builds to include the latter.
Linux now compiles the portable source directly; build-time string slicing is
removed. This sync is a fork prerequisite, not a proposed duplicate upstream
change. Shared game tables stay app-owned even though only Platinum is enabled.

## DSperate wrapper findings

- `ds::NDS` exposes main RAM, bus reads, ROM header/cart, two GPU framebuffers,
  `input::Frame`, SPU samples and state Reader/Writer. These are sufficient for
  the existing companion without SDL or JNI.
- Default software output is stacked 256x384 XRGB8888. The enhanced compositor
  converts to RGB565 and changes geometry by layout (max 512x384). Frontend
  scaling gives native top gameplay at exactly 4x on 1024x768; no viewport
  expansion or custom frontend panel option is involved.
- Input maps all twelve DS buttons and libretro pointer coordinates over the
  bottom half of the stack. The enhanced core reprojects bottom/side-by-side
  layouts and adds a controller pen. Slot2/CPAD/mic machinery stays in the
  wrapper; those peripherals are not validated in this milestone.
- Audio is 32728.5 Hz at 59.8261 fps. DSperate threading affinity and RT scheduling
  are not enabled: frontend pacing is authoritative. No new pacing thread added.
- Auto frameskip consumes RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK.
  NXRedux ma_environment.c explicitly lists callback 62 as TODO. The wrapper
  turns skipping off when registration fails. No equivalent hint exists in
  this examined minarch to turn on without frontend work.
- States have a u32 length followed by DSperate's state bytes, padded to a
  session-fixed allocation (~7.4 MB diagnostic / ~8.2 MB loaded Platinum).
  No UI trailer is added. Successful restore invalidates the companion snapshot.
- Battery RAM is exposed as RETRO_MEMORY_SAVE_RAM; Platinum is a listed 512 KiB
  chip. Save states keep its allocation stable in the tested session. Unlisted
  titles with initially unknown chip sizes retain the wrapper's limitation;
  not a Platinum blocker. A corrupt underlying state causes wrapper reset.
- NXRedux has RGB565/XRGB8888 support and queues dynamic geometry changes via
  ma_core/ma_avinfo. It owns saves, states, audio, shortcuts and postprocessing.
  Its native NDS pak uses DraStic, so it is not overwritten. The independent
  PDDS tag isolates saves/settings and can coexist with the working PokeDaisy pak.

## Android 1.4 reuse and limits

The latest pk_nds_field.cpp is already portable; pk_nds_field_gpu.cpp alone
needs melonDS. The shared touch rules distinguish Platinum top-only party and
options, general touch menus and command/move battle panels. The new Linux
core uses those bits directly and the native framebuffer one-colour fallback.
Android's full layout state machine lives in EmulatorEngine.kt and Compose,
including 2-frame on / 6-frame off settling and 12-lit / 90-frame fade waits.
Those animations and complete settling policy are not yet ported. We also do
not claim its custom battle-button renderer, touchscreen target discovery,
field actions, overlay styling or HG/SS/Gen5 parity. Manual screen selection
and native game input remain the fallback for every scene.

## Expansion research

No expansion is implemented in this libretro milestone. DSperate's current
software core still has fixed SCREEN_W*SCREEN_H framebuffers, line buffers and
native clipping/composition. It does not expose a wide output/camera API to
the wrapper. Scaling or stretching that output cannot reveal extra scenery.
The previous branch contains a separate single-pass projection experiment;
it targeted the older DSperate pin and is not silently applied here. Upstream's
melonDS wide renderer has emulator-specific polygon/clipping hooks and cannot
be called with a DSperate instance. Revalidate a constrained 3D projection hook
separately before promising more than native 4:3 gameplay.

## Executed checks

- Stock dsperate-lr wrapper: x86 interpreter and AArch64/JIT SDK builds; original
  diagnostic cart boots, states serialize/restore and main RAM restores exactly.
- Enhanced core: native CTest 3/3 (original diagnostic generation; frontend
  lifecycle/state/input/geometry smoke; captured Platinum companion + synthetic
  top-only/touch menu classification).
- AArch64 under qemu-aarch64 7.2: diagnostic lifecycle smoke and companion fixture
  pass. No QEMU timing is presented as hardware performance.
- Baseline -> enhanced -> baseline diagnostic states accepted in both directions.
- Private Platinum USA rev1: supplied 512 KiB raw save copied into frontend-owned
  SRAM; title/Continue/journal dismissed to reach live Jubilife overworld.
  Existing party, summary, bag, Dex and map views render from the live session.
- Native -> ARM/QEMU Platinum state loads and live frames run. Game, party, bag,
  Dex, map and manually selected bottom screen captured for comparison.
- Successful save-state restore preserves RAM exactly and SRAM pointer identity;
  too-short state container is rejected. This is not comprehensive corruption
  fuzzing, a game save/reload playthrough, or a sleep/quick-resume hardware test.
- Shared-object exports restricted to retro_*; ARM runtime dependencies are
  libc/libm/libdl/libpthread only, with maximum GLIBC_2.18 after static C++ linking.

No ROM, BIOS, raw memory fixture from this session, battery save or state is
included in the distributable pak. Screenshots are actual emulator output.

## Remaining physical gates

Launch via real NXRedux; test button pass-through, touchscreen dragging, all
Platinum native menu/battle transitions, raw in-game save/relaunch, frontend
state slots and quick resume, power/sleep, shaders, audio and thermal/frame-time
behavior. If performance needs skipping, first measure the problem scene;
adding a calibrated minarch buffer/lateness callback is a separate frontend
contribution. The non-PGO build needs measurement before changing CPU tuning.

Suggested contribution sequence: portable field-reader sync already upstream;
new Linux libretro baseline infrastructure; Platinum shared companion and input;
then layout/HUD refinements. Do not send a combined Android/other-game feature PR.
