# Unified native integration validation

Source base: own DS branch 89adcfe; pinned Thwonp feat/libretro-core 4ab40fa;
DSperate wrapper v3.3.0-lr.1 / emulator v3.3.0. No upstream history modified.

Executed:
- DS desktop CTest 5/5, including settings persistence/migration and previous
  shared-UI compatibility. The GB/GBA file cannot overwrite DS preference keys.
- Unified launcher: real shell execution against a fake minarch process checks
  .gb/.gbc/.gba/.nds dispatch, mixed case, spaces, unchanged ROM arguments,
  missing-file rejection and unsupported archive rejection.
- Both AArch64 cores build with the TrimUI SDK. Shared-object exports contain
  the expected retro_* functions only. Runtime versions: GB/GBA <= GLIBC_2.27,
  DS <= GLIBC_2.18, below the SDK/Brick target runtime.
- GB, GBC and GBA: original logo-free diagnostic carts run under AArch64/QEMU;
  serialize/run/restore succeeds and exposed SRAM pointer identity is retained.
  This harness supplies both full paths and bytes, respecting each core's
  content contract. mGBA exposes no SYSTEM_RAM through its wrapper here, so
  no zero-byte RAM comparison is presented as a memory-restoration check.
- DS: real Platinum USA rev1 copied raw save and prior native state load. Native
  and AArch64/JIT QEMU companion and Ring/Grid quick-menu captures are identical.
  Opening the quick menu leaves game RAM unchanged; A hands off to Party and B
  cancels. Grid is selected through the actual companion settings controls.
- The normal DS state format and firmware SRAM interface are unchanged.
- Projection research: three independent runs from the same copied Platinum
  field state at 100/90/80 percent produce different actual emulator outputs.
  The probe leaves the normal source/archive/core unchanged and is not the
  packaged core. Its reduced view is not asserted to work in battles, all maps,
  camera transitions, UI or presentation-neutral state restoration.

Outstanding hardware gates: launch all four formats from the new PDAI collection,
check X/Y and L2/R2 pass-through, existing-game SRAM import, frontend state slots
and quick resume, audio/performance and sleep. The owner verified the earlier
separate GB/GBA and DS implementations; that is not a hardware test of this new
combined pak. EXPAND/ZOOM OUT on GB/GBA and advanced HUD/field actions are inherited
from the pinned Thwonp implementation, not revalidated across all supported ROMs.

A future source merge must reconcile the portable UI versions. A future DS parity
milestone must port actual game-specific PC/Fly/Bike/Repel/Wild actions and gate
projection changes correctly, not relabel native image scaling as expansion.
