# Companion battle interactions — implementation checkpoint

This continues Phase 16 on phase16-nextui-checkpoints. Main's previous baseline
is retained. Native build passed once; pure-C captured-menu/model tests pass.
Real-ROM execution, final native/ARM regressions and packaging are in progress.
This checkpoint is not yet a verified hardware release.

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

Known current differences: exact Android animations/processing-ball and
held-frame switching are not yet translated; intermediate original party
menus may be visible. No Polished field item use/give/take was added.
