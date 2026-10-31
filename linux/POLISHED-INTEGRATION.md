# Polished Crystal on the current Linux host

The recovered companion now runs inside the latest-upstream-based Linux host.
Supported companion identities come from the app's Gen2PolishedCrystalGen.kt;
unknown ROMs retain emulation-only behavior. Expansion remains limited to the
verified old profile for normal 3.2.3 (CRC32 f98367e4).

## Controls

- Y while playing: current Linux quick-menu ring (PC, REPEL, BIKE and WILD on normal 3.2.3, save/load
  state, fast-forward and companion; other unavailable actions stay disabled).
- In the companion: L/R sections, D-pad navigation, A select, B back.
- X: return to game, except BAG where it toggles a favorite.
- Start: current Linux SETTINGS, including aspect, auto zoom and optional HUD.
  B or L/R returns from SETTINGS to the Polished companion.
- R2 on the companion BAG: open the selected ordinary or key item’s original actions (normal 3.2.3 only).
  Elsewhere R2 retains the load-state confirmation.
- HUD: party icons decoded from the identified ROM, levels, HP and status.
  GBA minimap/balls/repel HUD panels are hidden for Polished pending adapters.

The old party/summary, bag, map, dex, guide and read-only battle views are
reused. They have the old Polished layout, not Android's newest redesigned UI.
Normal 3.2.3 battle commands are connected through the controller-only
dispatcher described below. Ordinary item actions hand off to the game. Favorites load and save atomically per ROM
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
Preload DISPOSABLE_DATA/settings.ini with aspect=1 and hud=1. The default boot/outdoor route uses normal guest input without fixture RAM
or synthetic party data. The separate field-item fixture mode below adds
explicitly labelled disposable inventory for its positive tests.
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
still owned by the app. Fly still needs its own verified adapter;
the retail Crystal addresses are not used for Polished.
Original item actions are now accessible through the selected-item handoff below;
companion-driven item target/quantity confirmation remains unimplemented. Persistent favorites
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
holding an item, multi-count tossing, TMs are not claimed tested. Key-item fixture tests are described below.

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
still OFF, then enabled WILD and entered a normal wild battle (three steps initially,
20 on the latest repeat; encounter timing varies).
A normal input-earned battle state is retained privately for subsequent tests.
The accelerated guest-input helper calls the same production hold once per frame;
quick-menu activation and persisted settings are exercised through the real host.
These are emulator tests, not physical Brick validation. Trainer/scripted battles
and long playthroughs with WILD OFF still need integration coverage.

Reproduce after PC_OUTDOOR's earned state: PC_WILD=1 with --frames 22, then a
second process with PC_WILD=1 PC_WILD_RELOAD=1 and the same disposable data.
PC_BOOT and PC_SHOTS are required as for the other personal-ROM checks.


## Companion battle actions (normal 3.2.3)

During a battle, X opens the companion. L/R to BATTLE, then Select on the INFO
page opens CONTROLS. D-pad picks a cell; A confirms. Fight opens the companion
move grid after the game's move menu is visibly ready. B from that move grid
returns to the action grid. Y returns to battle information, X to the game,
and Start opens host settings as elsewhere.

Moves, Run and a successful switch return to the original game for animations,
dialogue and outcomes. Open the companion again on the next turn to issue
another command. Bag hands off to the game's original battle Bag: the user
chooses an item and target there. This does not automate turns, dialogue,
catching, item confirmations or the battle's outcome.

The existing BattleControlsScreen-inspired cells and party picker are reused;
no new artwork or layout system. Their requests now reach pocket_battle_begin
and pocket_battle_tick. Those emit ordinary bounded guest inputs and observe the
real menu cursor. Fight / Back reopen the companion only after six frames of
the expected visible menu, with a bounded timeout. B/X/Y/Menu or directions
cancel pending automation and return control. FF is suspended during a command
or pending menu handoff. Unsupported ROMs retain the read-only battle pages.

Removed the battle dispatcher's duplicate literal addresses and ROM identity:
all now come from the same pinned-symbol generator, cross-checked against the
app's Gen2Reader fields. Party stride, battle-mon size and move/PP offsets are
also derived from release symbols. No shared-core files changed.

Actual native and ARM/QEMU checks used an input-earned wild battle: INFO ->
CONTROLS -> Fight -> moves -> Back -> actions -> Fight -> Tackle. PP fell from
35 to 34 and foe HP from 13 to 7, then Run returned to the overworld. Separate
runs verified cancellation during a pending action and Bag handoff. PC storage
navigation and the existing GB/GBA diagnostic checks still pass.
Model tests additionally cover unavailable ROMs, empty/zero-PP moves, fainted /
egg / already-active party choices, timeout and cancellation. These model
checks do not establish successful switching. Live switch / forced replacement,
trainer battles, native battle item use, fast-forward interaction and physical
Brick behavior still need validation; the earned test party has one Pokemon.

Reproduce after PC_WILD_RELOAD earns wild-battle.ss: PC_BATTLE_CONTROLS=1 with
--frames 235, using the same PC_BOOT / PC_SHOTS and disposable data convention.
Add PC_BATTLE_BAG=1 for native Bag handoff, or PC_BATTLE_CANCEL=1 with
--frames 22 for cancellation. Tests observe the real UI / dispatcher state;
they do not set its selection, synthesize Pokemon or patch battle memory.


## Key items, Bike and Repel handoffs (normal 3.2.3)

R2 on the companion BAG now finds the highlighted key item as well as ordinary
items. The native key list is zero-terminated, not a quantity-pair pocket; it
has separate list and submenu validation. The controller finds its native slot,
scrolls normally and stops at the original Use / Sel / Quit menu (or Use / Quit
for non-registerable keys). It refuses unknown IDs, duplicate/malformed key
lists, changed slots and missing items. No key, cursor or registered-slot writes.
TM/HM selection still opens the whole native Bag; targeted teaching is unfinished.

Y -> BIKE opens the owned Bicycle's action menu. A confirms Use in the game;
this mounts or dismounts where the original game permits it. The tile says GET
OFF when riding. It does not borrow or replace a registered shortcut.
Y -> REPEL lists the owned kinds when there is more than one, then opens the
selected kind's original action menu. A confirms Use. While a Repel is active,
the tile shows its remaining steps and is disabled. No extra item is consumed
by reopening or trying the disabled shortcut. Both tiles are disabled without
the relevant owned item, or away from a free overworld. The game's own item
code owns environment restrictions and effects. These are handoffs, not automatic
item use. The app's existing BAG_ICON supplies a fallback satchel when item art
is unavailable to the quick menu; GBA item-art behavior is preserved.

Item IDs and Repel names come from ItemNamesPolishedCrystal.kt. Key capacity
and memory layout are app data cross-checked against pinned release symbols;
key submenu identity, player state and active Repel steps come from those
symbols. No handwritten address or item-ID table was introduced. No shared-core
files changed. Existing WILD tests now navigate the enlarged ring by real
D-pad inputs through quick_step rather than assuming WILD is the second tile.

Validation: 13/13 native and 12/12 ARM/QEMU CTest checks. Actual ROM/host tests
on both architectures used an explicitly labelled DISPOSABLE INVENTORY FIXTURE:
Bicycle placed ninth in a nine-key list, plus two Repels and two Super Repels.
The fixture changes private emulator WRAM before testing; these items were NOT
normally earned. After setup, real UI/controller inputs opened the ninth key's
native action menu, mounted and dismounted through user Use, and preserved all
four registered shortcuts. Companion R2 also found that scrolled key. The Repel
picker selected Super Repel; user Use set 200 steps and reduced its quantity
2 -> 1, leaving the other kind unchanged. An active Repel refused a second use.
Cancellation passed. These are actual emulation captures with fixture inventory,
not mockups or evidence of a naturally acquired Bicycle/Repel playthrough.

A separate normally earned new-game state (without added inventory) refused
both Bike and Repel shortcuts. Native PC, ordinary selected-Potion handoff,
remembered TM pocket, WILD persistence/grass and companion battle regressions
passed. Physical Brick operation, real progression inventories, unusual key
items and game-specific dismount restrictions remain unverified.

Optional reproduction: after PC_OUTDOOR normally earns live-test.ss, use
PC_FIELD_FIXTURE=bike|repel|key-r2|cancel with --frames 705. Each process needs
its own disposable --data directory, PC_BOOT and PC_SHOTS. It quits normally
once its assertions and capture complete. Capture names start with fixture-.
PC_FIELD_NO_ITEMS=1 with --frames 26 tests the earned no-item state without
inventory setup. Do not distribute any private game, save or state files.
