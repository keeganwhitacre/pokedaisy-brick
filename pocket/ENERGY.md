# Battery efficiency

Six to eight hours of active play is a goal, not a measured runtime claim.
This update reduces unnecessary host work and requests NextUI's own Auto CPU
policy. Only a real Brick discharge test can establish battery life at a given
brightness, volume, filter and fast-forward usage.

## What changed

- NextUI's launcher puts standalone paks in Performance mode. When its
  `$SYSTEM_PATH/bin/governor.sh` exists, our launcher now requests Auto before
  starting the existing application. The kernel can scale with workload. We
  neither ship a governor nor write CPU clocks/sysfs ourselves. NextUI owns
  the return-to-menu policy. No helper means the inherited policy remains.
- A paused core's decoded snapshot is reused until the core advances or a
  state load invalidates it. States with identical frame counters still refresh.
- Static Polished companion screens, host menus and quick-menu overlays redraw
  on input, data changes, notice appearance/expiry, display options, exposure,
  resize or texture restoration. Party bobbing retains its original 167ms
  phases and the map cursor retains its 333ms phases. No gameplay frames,
  input polls or emulated audio processing are removed.
- The quick menu avoids repeated framebuffer conversion, overlay uploads and
  state-file existence checks while idle. Companion filter masks stay cached.
- SDL audio is paused while gameplay is paused in the companion/quick menu,
  avoiding a device thread repeatedly consuming silence. Resuming queues the
  first new gameplay audio before unpausing; lifecycle reopen resets the state.
- Read-only CPU policy/frequency and battery percentage diagnostics run once
  at application start and exit when the NextUI platform adapter is active.
  Session logs include awake elapsed time, process CPU time, draws and snapshot
  reads. There is no battery polling thread, frame logging or new daemon.

Gameplay remains at the core's normal rate, with the existing fast-forward
choices. Expanded rendering, filters, original PPU pixels, controller mappings,
manual states, battery saves and recovery remain intact. Savings from paused
screens must not be represented as the same percentage improvement during
continuous gameplay or as an hours-of-runtime result.

## Compatibility and diagnostics

`POCKET_CPU_MODE=inherit` skips the helper. `performance` and `powersave` request
those modes from the same NextUI helper. Auto is the default; Powersave has not
been validated for full-speed gameplay/audio on hardware and is not selected
automatically. A failed helper request is logged and does not block launching.
Actual supported governor/frequency behavior depends on the installed firmware.

`POCKET_IDLE_CACHE=0` restores continuous paused redraw/snapshot behavior for
comparison or display-driver troubleshooting. Default caching keeps the usual
game-rate input/power polling, so it does not add a slow menu polling interval.
The generic non-Polished companion still redraws continuously; its paused data
snapshot is reused. Older display preference files and all existing save paths
are unchanged by this update.

## Verification and a useful hardware check

`pocket-efficiency-test ROM starter-state` runs identical controller sequences
with caching disabled/enabled, checks unchanged paused core counters, compares
the entire static canvas, and records process CPU time. It tests same-frame
state reload, texture/expose invalidation, notice expiry, original party/map
animation phases, and an idle quick menu. These are software tests on Linux
and the actual NextUI ARM binary under QEMU, not physical power measurements.

For the Brick, use a fixed comfortable brightness and the same display settings
for a substantial play session on each build. Avoid charging during comparison;
record fast-forward use and start/end battery percentage. Check that Auto is
actually active in `launch.log`, audio is clean, expansion stays smooth and
sleep really suspends. Battery percentage is coarse and nonlinear: a short
percentage sample cannot establish six-to-eight-hour endurance. A full run
from charged to low-battery under ordinary play is the meaningful final check.

## Platform reference

Inspected LoveRetro/NextUI commit
`5291344b3aceae5fa197437a70824afb41e2d338`:
`skeleton/SYSTEM/tg5040/paks/MinUI.pak/launch.sh` explicitly defaults launched
paks to Performance and restores policy after exit;
`skeleton/SYSTEM/tg5040/bin/governor.sh` owns Auto/Performance/Powersave.
No NextUI governor implementation is vendored into this GPL project.
