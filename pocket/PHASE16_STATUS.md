# Phase 16 recovery checkpoint

This branch preserves recovered work in progress after automated workspace
maintenance removed the previous local checkout and uncommitted Phase 16 work.
The published Phase 15 baseline remains unchanged on main.

The integrated checkpoint wires recovery and NextUI power backends into the
existing frontend, adds safe expanded dialogue and AUTO ZOOM IN, per-ROM AUTO
RESUME, a real-ROM session/lifecycle harness and optional direct-launch wrapper.

Rebuilt native and actual NextUI ARM/QEMU checks pass; VERIFICATION.md records
their scope. Captures and results have been regenerated after reconstruction.
The extra frontend corruption-fallback regression and final packaging are
being finished. This branch remains separate from the tested Phase 15 main.
Physical sleep/wake, audible audio and RTC require testing on a TrimUI Brick.
