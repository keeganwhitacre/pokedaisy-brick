# Phase 17: companion battle interactions

Bag follow-up: Android favorites are also ported, including per-ROM storage,
duplicate stacks, Default sorting and the conditional Favorites tab. This is
existing upstream Polished functionality, distinct from the downstream Bag
opener and from unimplemented item Use/Give/Take. X stars/unstars only while a
Bag description is open; its original companion toggle remains elsewhere.
The frontend test uses an inventory earned by normal Lyra battle completion
and the aide Potion gift at ElmsLab scene 5 coordinate (4,8), verified against
the actual v3.2.3 map source. No inventory is injected into game memory.

This continues Phase 16 on phase16-nextui-checkpoints. Main's previous baseline
is retained. Native/actual NextUI ARM builds and captured-menu/model tests pass.
A genuine normal-input Lyra battle verifies the second move's PP consumption,
the real battle Bag, Back and trainer Run refusal. The actual frontend's
Fight -> second move -> turn -> automatic return passes natively and on ARM/QEMU.
The genuine one-member party verifies active-slot refusal and picker cancel.
Optional/forced switching and game refusal are capture/model-tested, not yet
executed in a genuine multi-member real-ROM party. Physical controls still
require Brick testing. This is an incremental port, not full Android parity.

Source: checked-in Android BattleInputController.driveGbPickLocked,
driveGbSwitchLocked, readGen2BattleInput, readGen2PartyMenu, Gen2PartySwitch,
BattleControlsScreen.Gen1ActionButtons / Gen1MoveList / PartyPicker.
The actual Polished 3.2.3 release ram/wram0.asm and ram/wramx.asm confirm the
menu fields and party-action semantics. Menu addresses/geometry come directly
from GEN2_POLISHED_CRYSTAL. Initially enable only verified normal 3.2.3 SHA-1
6930b48af5844d373e3c9130f26d6dd1084cf4ed; other builds retain read-only screens.

The new battle_action module reads memory and emits ordinary controller bits.
It never writes cursor/game RAM or calls game routines. The filled 0xF0 cursor
tile must be visible, preventing stale menu variables from accepting presses.
FIGHT/BAG/PKMN/RUN keep the real 2x2 game order, moves its one-column order.
Navigation retries dropped inputs with source timing and bounded timeouts.
Party selection excludes fainted, egg and active slots; immutable identity is
looked up again on each step, including forced switches and game refusals.
Native identity retains species, OT ID and DVs as in gbMonKey, plus the third
Polished DV byte and personality/form bits as extra collision guards. This
avoids duplicating ROM species/form tables. No move or switch outcome is
fabricated: accepted cursor input is distinct from the game using a move;
switch completion requires the chosen identity in the actual active slot.

Brick adaptation: Start on Battle opens controls; D-pad/A select; PkMn uses
the existing party grid; Y returns to INFO, X to gameplay. The companion pauses
for selection, then shows the game performing the action and returns when the
next waiting menu is recognized. Bag hands control to the original game Bag.
Dialogue may still need ordinary game buttons. B/X/Menu/L3 or power cancel an
active sequence and release synthetic keys. Original read-only companion
pages, quick ring, save/recovery, expansion and filters remain in place.

Start+Select follows the same cancellation rule as Menu. A native/ARM frontend
test pauses a queued action through that hotkey, loads a state and verifies
that resuming cannot restart cancelled input. The isolated model/capture
harness passes ASan/UBSan (leak detection disabled); this is not a whole-core
sanitizer claim.

Known current differences: exact Android animations/processing-ball and
held-frame switching are not yet translated; intermediate original party
menus may be visible. No Polished field item use/give/take was added.

The real ROM drops B during move-menu opening. Back therefore retries only
while the same filled-cursor menu remains, bounded like other GB picks; it
does not report acceptance merely because a B pulse was queued. Android's
original Back queues one pulse. This is a reliability adaptation, not a new
game action. A fresh Compose preview attempt was blocked at Gradle download;
source-rendered Android screenshot comparison remains pending.

## Reproduce

Build POCKET_TESTS=ON. From pocket:

```
pocket-battle-test ../app/src/test/resources/fixtures
pocket-battle-runtime-test /private/normal-3.2.3.gbc /private/starter.ss
```

Generate the starter with the existing pocket-rom-test normal-input script.
The runtime harness finishes Elm's conversation, walks to the source-verified
scene-6 coordinate event at (4,6), then battles Lyra normally. Game coordinates
are read from the existing official-symbol-generated expansion mapping; no
RAM/fixture injection is used in these emulator tests. Separate pure-C model
tests deliberately mutate their own byte arrays to exercise failures.

The new module must be reviewed with Gen2Reader's menu geometry, cursor tile,
party-menu action values and Polished struct offsets on upstream updates.
BattleInputController SHA-256 at this port:
ad07035d1db292b58071b96a336ee3650aaff85e2e2d83e5d476b457cd5f26d5.
GbPartySwitch: bc5791a35ad51762f008b8c5dc5cce7a91ba2dd3fd195d29eb6362b157b49f63.
Gen2Reader: 8ec656a7687c9c9375964e5b4df7186abf6f4da6eb2b0b57d416d47b6d6e798c.
