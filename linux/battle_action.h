/* GPL-3.0. Android Gen2Reader/GbPartySwitch/BattleInputController translation.
 * Read-only observations and controller bits; no emulator/platform APIs. */
#ifndef POCKET_BATTLE_ACTION_H
#define POCKET_BATTLE_ACTION_H
#include "game_action.h"
enum pocket_battle_state {
  PB_NONE,
  PB_BUSY,
  PB_ACTION,
  PB_MOVE,
  PB_PARTY,
  PB_SUBMENU
};
enum pocket_battle_command {
  PB_CHOOSE_ACTION = 1,
  PB_CHOOSE_MOVE,
  PB_SWITCH,
  PB_BACK
};
struct pocket_battle_menu {
  enum pocket_battle_state state;
  int cursor, rows, count, out;
  bool forced;
};
struct pocket_battle_action {
  enum pocket_action_result result;
  enum pocket_battle_command command;
  unsigned frames, gap, hold, bits, presses, lastA, phase, phaseFrames;
  int target, pick, menu, lastCursor;
  unsigned char identity[8];
  bool sentA, switchSent, forced;
};
bool pocket_battle_supported(const char *sha);
bool pocket_battle_read(pocket_action_read, void *,
                        struct pocket_battle_menu *);
bool pocket_battle_begin(struct pocket_battle_action *, const char *sha,
                         enum pocket_battle_command, int value,
                         pocket_action_read, void *);
unsigned pocket_battle_tick(struct pocket_battle_action *, pocket_action_read,
                            void *);
void pocket_battle_cancel(struct pocket_battle_action *);
const char *pocket_battle_message(const struct pocket_battle_action *);
#endif
