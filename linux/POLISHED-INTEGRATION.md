# Polished Crystal integration checkpoint

Base: upstream dev 173c277d4f887848c83d23bbc12bfe747dc79895.
Recovered source: backup/complete-linux-20261025 at 1f3e1a1.

The Linux host now selects the old Polished 3.2.3 normal expansion profile
(CRC32 f98367e4). Other builds remain unsupported for expansion. The profile
comes from official release symbols; gen_polished_wide.py records and verifies
the symbol-file SHA256. No ROM assets are bundled. Current upstream GBA renderer,
follower reset and wild-encounter fixes are preserved.

The recovered read-only party/bag decoder is compiled and tested but deliberately
not connected to the companion: Gen 3 names/art would be wrong for Polished.
Its RAM constants and species/form/TM arrays are generated from the current app
sources by gen_polished_reader.py; --check detects stale output. This is an interim
bridge, not a completed migration of Gen 2 to gamedata.

Validation: native Linux build and 9 CTest checks passed, including current
upstream Polished captured-memory fixtures (normal 3.2.0–3.2.3, faithful 3.2.3).
No retail ROM, hardware, ARM/QEMU or visual integration validation in this checkpoint.

Next: identify supported ROMs by app-owned identity data; connect Gen 2 memory
access and game-specific names/art to the current UI; recover map/battle/dex
readers, nickname and summary fields. Held item and experience need a shared
snapshot extension. Check status encoding against Gen 2 before UI use. Then run
personal ROM/save integration and Brick checks. Keep unsupported actions disabled.
