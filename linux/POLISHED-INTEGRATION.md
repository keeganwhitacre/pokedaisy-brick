# Polished Crystal on the current Linux host

The recovered companion now runs inside the latest-upstream-based Linux host.
Supported companion identities come from the app's Gen2PolishedCrystalGen.kt;
unknown ROMs retain emulation-only behavior. Expansion remains limited to the
verified old profile for normal 3.2.3 (CRC32 f98367e4).

## Controls

- Y while playing: current Linux quick-menu ring (save/load state, fast-forward,
  companion; unavailable GBA actions stay disabled or absent).
- In the companion: L/R sections, D-pad navigation, A select, B back.
- X: return to game, except BAG where it toggles a favorite.
- Start: current Linux SETTINGS, including aspect, auto zoom and optional HUD.
  B or L/R returns from SETTINGS to the Polished companion.
- HUD: party icons decoded from the identified ROM, levels, HP and status.
  GBA minimap/balls/repel HUD panels are hidden for Polished pending adapters.

The old party/summary, bag, map, dex, guide and read-only battle views are
reused. They have the old Polished layout, not Android's newest redesigned UI.
Battle action injection and bag use shortcuts are not enabled. Favorites load and save atomically per ROM SHA-1 under DATA/favorites/. No DS or Seaglass work is included.

## Architecture

Linux-specific companion code stays in linux/. gen_polished.py regenerates
configuration, names and guide tables from current app sources. It and its
--check test replace the interim duplicate reader generator. No new handwritten
memory-address list is introduced. This remains a bridge; the full Gen 2
migration to gamedata has not been implemented.

Shared dependencies recovered from the old build: optional snapshot fields and
larger bag capacity, font overrides, newline wrapping, middle-dot glyph and
upstream-generated EXP tables. These should be reviewed separately before an
upstream PR. Existing GBA decoding/rendering code is preserved.

## Verification

Native: 12/12 CTest checks. ARM cross-build: successful; QEMU: 11/11 checks.
Includes upstream RAM fixtures for Polished 3.2.0–3.2.3 and faithful, bag capacity,
party/forms/eggs, battle/guide navigation and drawing, EXP and generated-table
freshness, plus existing GB/GBA diagnostic runtime and packaging checks.

Live native validation now uses the supplied 3.2.3 ROM and a disposable copy of
the supplied save. Normal guest input starts a new game, reaches Elm's lab and
obtains Cyndaquil. Actual host captures cover companion pages, expansion, quick
menu, party HUD and Settings. State roundtrip restores the frame counter; HUD
and EXPAND settings survive save/reload. Fixed X double-processing and cropped
legacy canvas found by this run. This does not validate a resumed playthrough:
the supplied save did not resume one. Physical Brick testing remains outstanding.

Optional runner: build with -DPOCKET_LIVE_TESTS=ON; set PC_BOOT to
linux/boot-polished-newgame.txt and PC_SHOTS to an existing capture directory.
Run linux-polished-live-test --data DISPOSABLE_DATA --frames 90 ROM.
Preload DISPOSABLE_DATA/settings.ini with aspect=1 and hud=1. All guest input is
normal controller input; no fixture RAM or synthetic party is injected.
Do not replace a known-good installation without backing it up and its saves.

NPC audit: optional linux-polished-objects-test ROM STATE checks original PPU
pixel preservation and ROM/VRAM graphics against a live state, plus explicitly
labelled snapshot-model cases for masks/time/association/palette/special sprites.
PC_PROBE=1 remains a short lab movement diagnostic. Use PC_OUTDOOR=1 to
normally earn the aide-gift/outdoor state, then POCKET_TEST_OUTDOOR=1 on the
object test to exercise both map crossings.


## Outdoor and missing-action follow-up

Normal guest input now completes Lyra's battle and exits Elm's lab when
PC_OUTDOOR=1. The live runner can also load its own previously earned state
(PC_RESUME=1); PC_FAVORITES=1 tests X in the actual bag, and a second process
with PC_FAVORITES_RELOAD=1 verifies the stored favorite survives restarting.
The outdoor object test crossed New Bark Town -> Route 29 -> New Bark Town,
checking every original PPU pixel each frame. Captures showed no obvious
phantom/duplicate person in these samples. This is limited map coverage;
special/unloaded-palette sprites are still omitted, so NPC pop-in is possible.

Current upstream Android GbField.forGame selects retail Crystal, not Polished,
for its Gen 2 PC/Fly/Wild implementation. Those actions cannot be enabled just
by connecting a Linux menu. The old game_action.c offers a controller-only
open-Bag prototype, but its handwritten addresses are not in the current app
configuration. It was not imported as a second address list. A future action
adapter needs generated, version-verified data and completion/failure checks.
Item use/give/toss and battle commands remain disabled. Persistent favorites
was the first independent feature restored in this follow-up.
