# Portable large-canvas Summary

Reference: Android MonDetailScreen.kt SummaryLeftColumn, InfoSection,
MatchupSection and MovesSection. The first native pass follows equal-width
information/move columns, four move rows, type names, power/PP and grouped
defensive type matchups. It uses existing generated species/move/chart tables;
no new addresses, independent type charts or bundled game artwork are added.

This is a shared C candidate, isolated from Linux integration on its own branch.
It is enabled only on canvases at least 480x320; the original 320x240 3DS and
egg summary layouts retain their previous path. The Linux menu composition is
not changed. The actual supplied Emerald save is used for runtime captures.

Remaining Android fidelity: colored type chips/stripes, multiplier labels,
EXP/NEXT, held-item details, Learn pages, party-strip controls, detail
interactions and exact frame/typography metrics. Matchup groups are bounded by
the available height; this initial layout is not a scrolling implementation.
Do not describe this as full Summary parity. No original Android screen is
replaced and no Brick macros or platform imports enter the shared renderer.

## Read-only retail Gen 3 STATS page

The next increment translates Android MonStatsSection.kt and MonStats.kt.
STATS replaces the four move rows; MOVES returns to them. It shows computed
HP/Attack/Defense/Sp.Atk/Sp.Def/Speed, IVs and EVs with totals, nature effects,
and retail Gen 3 Hidden Power type/power. Values are decoded from the original
100-byte encrypted Pokemon struct using Gen3Mon.kt's verified retail offsets.
The game owns the computed stats: this does not recompute them from newer EVs.
It also fills the existing experience and heldItem fields, but does not yet
claim an EXP progress bar or held-item actions. No writes to game memory occur.

Nature names and Hidden Power's type order are generated from Android's own
MonStats.kt / TypeChartUtil.kt by `python3 3ds/tools/gen_stats.py`; `--check`
detects drift. Nature/Hidden Power formulas are direct portable translations.
The optional stats block has an explicit validity flag. Unknown decoders and
eggs do not expose STATS. The compact 320x240 path is unchanged. Crystal's
separate companion is unchanged and does not claim Gen 3 stats support.

Controller adaptation: use the existing D-pad focus and A on STATS/MOVES;
B closes Summary. A newly opened Summary starts on MOVES. Android's colored
nature arrows are represented by red + / blue - text until its pixel icons
are shared. IV bars use Android's thresholds and colors (gold at 31); they
are omitted below 220 pixels of column width, retaining all numeric values.
The fixed table fits the supported 480x320 minimum without scrolling. It is
not a claim of exact Compose typography, colored type badges or full parity.

### Verification

`make -C 3ds test-stats test-host test-font preview` passes. The STATS test
checks all 24 independently generated encryption permutations, checksum
failure, IV/EV/computed-stat extraction, nature and Hidden Power endpoints,
captured Emerald/FireRed RAM, toggle controls and egg/unknown/compact guards.
Screens were inspected at 480x320, 512x384 and at the actual SDL 1024x768
size. All 80 existing compact preview PNGs remain byte-for-byte identical.

Native Linux and actual NextUI AArch64 frontend/test builds pass. Native and
ARM/QEMU decoder suites pass, including the existing Crystal fixtures. The
supplied Emerald ROM/save runtime test opens STATS through the real hit target
and captures the result alongside its expansion/zoom/menu/state checks. This
is Linux/QEMU verification; the new STATS interaction awaits physical Brick
testing. The user's ROM and save remain private and are never committed.
