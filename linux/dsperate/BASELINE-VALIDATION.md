# DS baseline on the Polished foundation — 2026-10-31

## Recovery and preserved work

Fresh clone of keeganwhitacre/pokedaisy-brick; no existing checkout was assumed.
The initial working tree was clean. All remote branches and recent checkpoint,
backend and experiment history were inspected before integration. A prior
Polished NextUI zip and personal Polished files survived elsewhere in scratch;
the DS ROM/save were recovered from the user's existing files for private tests.
No previous binary was used to build this candidate.

Base: ec441a754f8de4cd231dbc126ef9ac65e2bd5992, origin/brick/dev.
Preservation tag: brick/polished-foundation-ec441a7 (annotated; peeled target
verified against the exact base). One new development branch: brick/ds-baseline.
No upstream branch or existing experiment/PR branch was changed.

Recovered implementation: pr/linux-ds-backend at 6656fef and the separate shared
Platinum metadata prerequisite 30e9504. Existing upstream PRs #323 and #324 are
open and unmerged. The larger experiments at experiment/dsperate-linux and
experiment/ds-one-screen-battles remain available. The auxiliary expansion,
overlapping battle panel and custom battle-command experiment were excluded.

Upstream main inspected at 52f9597; current upstream dev inspected at
173c277d4f887848c83d23bbc12bfe747dc79895. Its DS architecture uses melonDS,
app-owned Kotlin Gen 4/5 readers/tables, and portable C++ field readers. The
Polished base already contains its additional White/Black 2/White 2 field data;
this integration does not roll that back or advertise Linux support for it.
The existing generated Platinum bridge and shared field callbacks are reused;
no second hand-maintained address registry was introduced. RAM-only extraction
from pk_nds_field.cpp is retained as an explicitly temporary build boundary.

DSperate is pinned, unmodified, at e04b806b5c01527b08a8e7181476ada57e97db52.
The optional backend requires C++17 and ships DSperate GPL-3+, FreeBIOS BSD,
miniz MIT and both bundled font notices. It uses built-in FreeBIOS/generated
firmware; packages contain no commercial BIOS. mGBA remains pinned at
26b7884bc25a5933960f3cdcd98bac1ae14d42e2. ARM uses JIT/NEON; native x86 uses
the interpreter. No broad upstream refactor or dependency update was attempted.

## Polished behavior checked in code

The original mGBA/Polished emulation loop and core files are unchanged. main.c
adds only conditional .nds dispatch, a controller-event callback and .nds picker
recognition. HUD conflict resolution preserves the existing Polished monIcon
callback and adds a separate optional DS callback.

Verified paths retained: Polished companion and expansion; quick menu and
Ring/Grid layout; optional HUD; LCD mask; quick resume/save-on-quit; recovery
bypass; native PC and Fly actions; WILD; automatic Bike/Repel; companion Potion
targeting. Relevant code is in linux/main.c, polished*.c/inc, game_action.c,
field_items.c, fly.c, battle_action.c, quick_menu.c, hud.c, display_filter.h and
3ds/core/pd_settings.c. Passing fixture/unit suites do not replace fresh physical
Polished gameplay validation of this combined candidate.

## Integration and safety changes

- Optional DS backend on the exact base; DS expansion is absent.
- Native 256x192 screen at 4x on 1024x768; companion 341x256 at 3x, centered.
  Renderer logical/integer scaling and viewport are reset after the launcher.
- Four DS face buttons retained; L2 quick menu, R2 screen swap, Select+D-pad/A
  stylus. Mapped-controller hotplug reaches the host; disconnect pauses/clears
  keys. Focus loss/minimize pauses; an offscreen surface's initial hidden event
  does not pause. Real NextUI power/sleep behavior remains untested.
- Existing shared Ring/Grid renderer, party HUD, LCD mask and pd_settings
  persistence are used. DS companion/Settings is still its simpler UI. Only DS
  screen/battle preference is separate. No DS field actions or FF added.
- Manual save/load/exit use cancel-first confirmations. Manual corrupt-state
  loading validates a separate console before replacing the current one. ARM's
  global JIT is detached/rebound in the proper order during a successful load.
- Existing bounded save backups, settled battery writes and atomic replacement
  retained. Quick resume and save-on-quit are independent shared preferences;
  --no-resume suppresses automatic loading/capture and preserves last-game data.
- Ordinary play uses accumulated 59.8261-Hz deadlines. Gameplay companion reads
  are throttled to one per 12 iterations, with refresh on entry. No claim of
  physical speed or battery improvement follows from these implementation choices.

## Executed checks

Native Release: 19/19 CTest checks passed. TrimUI AArch64 Release: 18/18 passed
under qemu-aarch64 10.0.13 with the pinned NextUI SDK sysroot. PortMaster launcher
execution is intentionally native-only. New NextUI packaging smoke checks also
passed for AArch64. Rebuilds used the pinned cores, no downloaded old binary.

The original diagnostic NDS cart is a fixture, not a commercial game. Tests cover
actual launcher/backend execution, four visible source-corner markers at
1024x768, manual top/bottom swaps, overlay pause/input handling, cancel-first
confirmation, settings changes through actual host controls, valid state loads,
empty/invalid/truncated manual and resume states, restart capture/loading,
--no-resume and environment bypass, and newer-battery-save precedence. Display
state checks also exercise 1280x720 and 640x480. Captured upstream RAM fixtures
verify Platinum party/bag/dex/map and battle pointer-chain metadata. The captured
battle fixture's missing command head is explicitly seeded in that test; it is
not presented as a live battle capture.

Private Platinum USA rev1 tests on both native and ARM/QEMU:
- Load the user's raw battery save, boot and continue to the overworld.
- Resume the same real game state, exercise companion party/bag/dex/map and
  physical host input scripts, and capture actual SDL output at 1024x768.
- ROM-backed party icons (including Burmy/egg) and Town Map asset checks pass.
- Native and ARM/QEMU field/party/map/Grid screenshots from the same state are
  pixel-identical. Screenshot names state which runtime produced them.

There has been no physical Brick test in this run. Sustained real-device speed,
audio quality, power-button suspend/recovery, long sessions and in-game battery
save round trips remain hardware validation items. Platinum companion is
read-only and restricted to the exact app-identified USA rev1 ROM. Other DS games,
including Black 2, are not validated Linux support merely because loading exists.
No ROM, private save/state, BIOS or extracted art is committed or packaged.
