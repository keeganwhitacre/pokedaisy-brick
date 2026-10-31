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
- R2 on the companion BAG: open the selected ordinary item’s original actions (normal 3.2.3 only).
  Elsewhere R2 retains the load-state confirmation.
- HUD: party icons decoded from the identified ROM, levels, HP and status.
  GBA minimap/balls/repel HUD panels are hidden for Polished pending adapters.

The old party/summary, bag, map, dex, guide and read-only battle views are
reused. They have the old Polished layout, not Android's newest redesigned UI.
Battle commands are not enabled in the companion. Ordinary item actions hand
off to the game as described below. Favorites load and save atomically per ROM
SHA-1 under DATA/favorites/. No DS or Seaglass work is included.

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

Native: 13/13 CTest checks. ARM cross-build: successful; QEMU: 12/12 checks.
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
Original item actions are now accessible through the selected-item handoff below;
companion-driven target/quantity confirmation and battle commands remain disabled. Persistent favorites
was the first independent feature restored in this follow-up.


## Selected-item actions

R2 in the companion BAG locates the highlighted ordinary item, switches to its
real pocket, selects its actual inventory slot, and stops at the original game’s
Use/Give/Toss menu (or the subset the game allows). The player chooses the action,
party target and any quantity/swap/toss confirmation using normal game controls.
This is not a companion-rendered target chooser or automatic confirmation.

Only normal 3.2.3 is enabled. The item ID is resolved from the companion’s sorted
or favorite-filtered list. A unique ordinary stack is required; duplicate IDs,
changed quantities, malformed lists and unavailable menu states are rejected.
Empty bags, TMs and key items retain the original Bag handoff. The TM/HM screen
is not a verified completion state and may time out if it was remembered.

All observations are generated from pinned official release symbols, with
app-owned addresses/capacities and the app’s ROM identity cross-checked. The
controller has no game-memory write API. The host uses one frame per iteration
during automation even in FF. B/X/Y/Menu/directions cancel, and bounded timeouts
release input. It never selects Use/Give/Toss or accepts a toss/swap on its own.

Validation: 13 native and 12 ARM/QEMU tests, plus actual native and ARM/QEMU item
runs. On separately restored earned states: Potion use healed Cyndaquil 18->21 HP
and quantity 1->0; use at full health kept quantity 1; Give moved the Potion into
the held-item slot; Toss consumed it after the game’s own confirmation. No ROM,
HP, inventory or event-flag injection was used. Damage was earned in a wild
battle using normal controller inputs. Wider item/party/version coverage and
physical Brick testing remain outstanding. Held-item replacement when already
holding an item, multi-count tossing, key items and TMs are not claimed tested.

Optional runner: PC_HANDOFF=1 loads earned live-test.ss, opens the real companion,
navigates to BAG and presses R2; it verifies item identity and submenu. Add
PC_HANDOFF_CANCEL=1 for cancellation; PC_ITEM_ACTION=use|give|toss to complete
an action with normal game controls. PC_INJURE=1 normally reaches Route 29 grass,
earns damage, escapes and saves private injured.ss; PC_HEAL=1 plus the use mode
loads that state. These routes assume the generated Cyndaquil starter fixture,
not an arbitrary personal playthrough. Tests never publish ROMs/saves/states.
