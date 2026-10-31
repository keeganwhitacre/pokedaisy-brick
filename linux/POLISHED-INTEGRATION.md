# Polished Crystal on the current Linux host

The recovered companion now runs inside the latest-upstream-based Linux host.
Supported companion identities come from the app's Gen2PolishedCrystalGen.kt;
unknown ROMs retain emulation-only behavior. Expansion remains limited to the
verified old profile for normal 3.2.3 (CRC32 f98367e4).

## Controls

- Y while playing: current Linux quick-menu ring (PC and WILD on normal 3.2.3, save/load
  state, fast-forward and companion; other unavailable actions stay disabled).
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

The Polished field adapter now reuses Android's Crystal queued-script mechanism
and the unmodified shared pk_gb_field.c writer. Its PC entry and observations
are generated from the pinned official 3.2.3 symbols, with the ROM identity
still owned by the app. Fly, Repel and Bike need their own verified adapters;
the retail Crystal addresses are not used for Polished.
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
Empty bags, TMs and key items retain the original Bag handoff. The remembered TM/HM screen is recognized by its verified waiting-menu geometry
and filled cursor, so returning to an ordinary item does not time out. Targeted
TM teaching is still not implemented.

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


### Remembered TM pocket regression

PC_HANDOFF_TM=1 with PC_HANDOFF=1 (235 frames) opens the game Bag, navigates to
its empty TM/HM pocket, captures it, closes both Bag and Start menu, then uses
the actual companion R2 action to find the Potion. This passed natively and under ARM/QEMU. The
model regression rejects stale TM geometry when the filled cursor is absent.
Menu addresses and the cursor glyph are generated from pinned official symbols
and app-owned data; no new handwritten memory table is added.

Key-item audit: Gen2Reader.kt stores Polished keys as a zero-terminated list of
up to keyItemsMax=38 IDs, with companion namespace 0x200. These cannot be fed to
the ordinary count/{id,quantity} parser. Key items retain the original Bag handoff
until an earned inventory, current-key identity and context-specific submenu
have live validation. Neither targeted key-item use nor TM teaching is claimed.


## PC anywhere (normal 3.2.3)

Face up on the overworld, press Y, choose PC and press A. Advance the original
text with A, then use Bill's PC or the player's PC normally. The game owns
storage, item actions, summaries, release confirmations and legality checks.
B cancels; leaving Bill's storage asks whether to continue box operations.
No companion box editor or automatic deposit/withdraw/release is implemented.

The quick-menu tile explains why it is unavailable during battles, menus,
scripted events, queued events, before obtaining a Pokemon, or while facing
another direction. Recheck all guards at activation. Write the queue payload
first and publish the flag last, between frames, through the existing shared
GB adapter. No new CPU calls, script bytes or manually maintained offsets.
The original PC script's facing check is respected rather than bypassed.
The host hides expansion and HUD during the requested PC session and restores
them on return to the overworld.

Polished save states now capture and restore battery memory with working RAM.
This is necessary for its SRAM-access integrity counter and boxed Pokemon.
Loading a state also rewinds in-game save progress to that snapshot. Legacy
states without SRAM are refused before changing the core; resume an in-game
save and create a new state. Other games' state policy is unchanged.

Actual-ROM native and ARM/QEMU checks: quick-menu activation; PC top-menu
identity; Bill's storage; all three cursor modes; switching boxes and back;
party Pokemon action menu; game's refusal to deposit the last healthy Pokemon;
clean exit with party intact. A fresh normally earned state roundtrip succeeds;
legacy-state rejection leaves the frame counter unchanged. The remembered-TM
Bag handoff still reaches the selected Potion's original action menu.
Successful Pokemon deposit/withdraw, player's item storage, an established
playthrough's boxes, and physical Brick operation still need validation.

Reproduce the PC check after earning the outdoor state with PC_OUTDOOR=1:
set PC_STORAGE=1, PC_BOOT and PC_SHOTS, then run linux-polished-live-test with
--data DISPOSABLE_DATA --frames 24 ROM. It uses the earned live-test.ss and
normal guest inputs. Only the actual feature writes the verified script queue;
the test does not inject Pokemon, items or fixture memory. Do not distribute
its ROM, battery saves or states.


## Wild encounters (normal 3.2.3)

Y -> WILD toggles random walking encounters on/off using the existing host
wildoff setting. OFF holds the game's five-step wild encounter cooldown before
each emulated frame, including every fast-forward frame, through the shared GB
writer. It skips title/non-field states, native menus and battles. Scripted and
trainer battles are not disabled. ON stops holding; the remaining short cooldown
expires normally. No encounter-rate, map or inventory tables are changed.

Native and AArch64/QEMU supplied-ROM tests each walked 200 actual Route 29 grass
steps without random battles while OFF, restarted and verified the setting was
still OFF, then enabled WILD and entered a normal wild battle after three steps.
A normal input-earned battle state is retained privately for subsequent tests.
The accelerated guest-input helper calls the same production hold once per frame;
quick-menu activation and persisted settings are exercised through the real host.
These are emulator tests, not physical Brick validation. Trainer/scripted battles
and long playthroughs with WILD OFF still need integration coverage.

Reproduce after PC_OUTDOOR's earned state: PC_WILD=1 with --frames 18, then a
second process with PC_WILD=1 PC_WILD_RELOAD=1 and the same disposable data.
PC_BOOT and PC_SHOTS are required as for the other personal-ROM checks.
