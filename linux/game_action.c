#include "game_action.h"
#include <string.h>
#include "polished_action_data.h"
/* Observation values are generated from app data and pinned official symbols. */
static bool read8(pocket_action_read r, void *c, uint32_t a, uint8_t *v) {
  return r(c, a, v, 1);
}
static bool field(pocket_action_read r, void *c) {
  uint8_t state, script, map, battle;
  return read8(r, c, PD_PC_STATE, &state) && read8(r, c, PD_PC_SCRIPT, &script) &&
         read8(r, c, PD_PC_MAP, &map) && read8(r, c, PD_PC_BATTLE, &battle) &&
         !(state & 0x40) && script == 0 && map == 2 && battle == 0;
}
bool pocket_action_begin(struct pocket_action *a, const char *sha,
                         pocket_action_read r, void *c) {
  if (a->result == PA_RUNNING)
    return false;
  *a = (struct pocket_action){.result = PA_REJECTED, .target = 2};
  if (!sha || strcmp(sha, PD_PC_ACTION_SHA) ||
      !field(r, c))
    return false;
  a->result = PA_RUNNING;
  return true;
}
unsigned pocket_action_tick(struct pocket_action *a, pocket_action_read r,
                            void *c) {
  if (a->result != PA_RUNNING)
    return 0;
  if (++a->frames > 360) {
    a->result = PA_TIMEOUT;
    return 0;
  }
  /* Two frames pressed, then release while the game's normal menu opens. */
  if (a->phase == 0) {
    if (a->frames <= 2)
      return 8;
    a->phase = 1;
  }
  if (a->pulse) {
    --a->pulse;
    return 0;
  }
  uint8_t ptr[2], state, script, battle;
  if (!r(c, PD_PC_POINTER, ptr, 2) || !read8(r, c, PD_PC_STATE, &state) ||
      !read8(r, c, PD_PC_SCRIPT, &script) || !read8(r, c, PD_PC_BATTLE, &battle)) {
    a->result = PA_REJECTED;
    return 0;
  }
  unsigned menu = ptr[0] | (ptr[1] << 8);
  if (battle) {
    a->result = PA_REJECTED;
    return 0;
  }
  if (a->phase == 2) {
    bool pack = false;
    for (unsigned i=0;i<sizeof pd_pc_pack_menus/sizeof *pd_pc_pack_menus;i++)
      pack = pack || menu == pd_pc_pack_menus[i];
    if (pack && (state & 0x40) && script == 1) {
      if (++a->stable >= 6)
        a->result = PA_DONE;
    } else
      a->stable = 0;
    return 0;
  }
  uint8_t list[11], cursor;
  if (menu != PD_PC_START_MENU || !(state & 0x40) || script != 1) {
    a->stable = 0;
    return 0;
  }
  if (!r(c, PD_PC_LIST, list, sizeof list) || !read8(r, c, PD_PC_CURSOR, &cursor))
    return 0;
  unsigned n = list[0];
  if (n < 1 || n > 9 || cursor < 1 || cursor > n || list[n + 1] != 255) {
    a->stable = 0;
    return 0;
  }
  unsigned seen = 0, row = 0;
  for (unsigned i = 1; i <= n; i++) {
    if (list[i] > 8 || (seen & (1u << list[i]))) {
      a->result = PA_REJECTED;
      return 0;
    }
    seen |= 1u << list[i];
    if (list[i] == a->target)
      row = i;
  }
  if (!row) {
    a->result = PA_REJECTED;
    return 0;
  }
  if (++a->stable < 6)
    return 0;
  a->stable = 0;
  a->pulse = 12;
  if (cursor == row) {
    a->phase = 2;
    return 1;
  } // Normal game A, never change cursor RAM.
  return cursor < row ? 128 : 64;
}
void pocket_action_cancel(struct pocket_action *a) {
  if (a->result == PA_RUNNING)
    a->result = PA_CANCELLED;
}
const char *pocket_action_message(const struct pocket_action *a) {
  switch (a->result) {
  case PA_DONE:
    return "Original game Bag opened";
  case PA_REJECTED:
    return "Bag shortcut needs verified 3.2.3 free overworld";
  case PA_TIMEOUT:
    return "Bag shortcut timed out; game control restored";
  case PA_CANCELLED:
    return "Shortcut cancelled; game control restored";
  default:
    return "Opening original game Bag - B cancels";
  }
}
