# Portable large-canvas Summary, first translation

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
EXP/NEXT, held-item details, Stats/Learn pages, party-strip controls, detail
interactions and exact frame/typography metrics. Matchup groups are bounded by
the available height; this initial layout is not a scrolling implementation.
Do not describe this as full Summary parity. No original Android screen is
replaced and no Brick macros or platform imports enter the shared renderer.
