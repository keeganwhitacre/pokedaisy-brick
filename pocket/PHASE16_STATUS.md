# Phase 16 recovery checkpoint

This branch preserves recovered work in progress after automated workspace
maintenance removed the previous local checkout and uncommitted Phase 16 work.
The published Phase 15 baseline remains unchanged on main.

The second checkpoint wires the reconstructed recovery-session and NextUI
power backends into the frontend and build, restores the expanded dialogue
option, and adds a real-ROM session/lifecycle harness and direct-launch wrapper.
Builds/tests are in progress. This remains a checkpoint, not a verified release.
Earlier native/ARM results and screenshots cannot certify these reconstructed
files; those local logs and captures were removed with the workspace.

Next: finish native and actual NextUI ARM
builds and ROM regressions, inspect captures, and publish further checkpoints.
Physical sleep/wake and power behavior require testing on a TrimUI Brick.
