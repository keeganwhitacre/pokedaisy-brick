# Large-canvas Summary information

This focused shared-core feature builds on the existing large-canvas Summary
and optional STATS page. At 480x320 and larger it adds total EXP, the current
level's progress bar, NEXT remaining EXP, MAX at level 100, and the held item.
The decoder explicitly declares whether EXP/item data is available: unavailable
fields are hidden rather than shown as zero or NONE. Eggs keep the legacy path.
The 320x240 composition remains unchanged.

The default INFO view uses the left column for progress and the held item.
TYPES restores the existing grouped defensive matchups; INFO returns. ITEM
opens a read-only description in the right column, using the same game-specific
names/descriptions as Bag. CLOSE or B returns to the previous moves/stats pane;
a second B returns to Party. STATS from item details opens STATS immediately.
The arrow buttons select adjacent party members with wraparound, reset to
INFO/MOVES, and show the current slot/count. All controls use ordinary portable
hit targets, so the Linux host's existing D-pad/A focus navigation works.
Renamed Pokemon also show their species under the level when space permits.

`tools/gen_experience.py` generates all six curves and retail internal-species
growth rates directly from Android ExpTables.kt. It checks the Android
SnapshotView.kt policy marker and records source hashes. `--check` detects drift.
The shared calculator is independent of Linux; adapters with other species
numbering must supply their own growth mapping. No ROM addresses, game-data
architecture refactor, emulator dependencies or item writes are introduced.

Validation: `make -C 3ds test-host test-font test-stats preview`. The summary
test covers all six curves at every level, progress clamps, NEXT/MAX, invalid
arguments, retail growth mapping, data availability, item/back/stats transitions,
party wraparound, eggs and the compact path, on captured Emerald/FireRed RAM.
The 24 encryption-order checks and optional stats tests remain. Generated
fixture captures are explicitly named `held-item-fixture`; they are not live
emulator captures. Device controller ergonomics require hardware testing.

Remaining Android differences include item artwork, TAKE/GIVE actions, move
detail/learn pages, full type-matchup pagination, colored type badges and the
Android party strip. They are separate features, not implied by this change.
