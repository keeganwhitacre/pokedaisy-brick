#include "game_action.h"
#include <string.h>
/* Rangi42/polishedcrystal tag v3.2.3 and official release .sym:
 * wStateFlags d0f5, wScriptMode d436, wMapStatus d431, wBattleMode d233,
 * wMenuItemsList d03e (WRAM1); wMenuDataPointer ce61, wMenuCursorY ce84.
 * StartMenu.MenuData 04:6160; pack pocket MenuData2 04:4791..47f1.
 * See ACTIONS.md for source provenance and fail-closed limitations. */
static bool read8(pocket_action_read r, void *c, uint32_t a, uint8_t *v) {
  return r(c, a, v, 1);
}
static bool field(pocket_action_read r, void *c) {
  uint8_t state, script, map, battle;
  return read8(r, c, 0x1d0f5, &state) && read8(r, c, 0x1d436, &script) &&
         read8(r, c, 0x1d431, &map) && read8(r, c, 0x1d233, &battle) &&
         !(state & 0x40) && script == 0 && map == 2 && battle == 0;
}
bool pocket_action_begin(struct pocket_action *a, const char *sha,
                         pocket_action_read r, void *c) {
  if (a->result == PA_RUNNING)
    return false;
  *a = (struct pocket_action){.result = PA_REJECTED, .target = 2};
  if (!sha || strcmp(sha, "6930b48af5844d373e3c9130f26d6dd1084cf4ed") ||
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
  if (!r(c, 0xce61, ptr, 2) || !read8(r, c, 0x1d0f5, &state) ||
      !read8(r, c, 0x1d436, &script) || !read8(r, c, 0x1d233, &battle)) {
    a->result = PA_REJECTED;
    return 0;
  }
  unsigned menu = ptr[0] | (ptr[1] << 8);
  if (battle) {
    a->result = PA_REJECTED;
    return 0;
  }
  if (a->phase == 2) {
    bool pack = (menu == 0x4791 || menu == 0x47a9 || menu == 0x47c1 ||
                 menu == 0x47d9 || menu == 0x47f1);
    if (pack && (state & 0x40) && script == 1) {
      if (++a->stable >= 6)
        a->result = PA_DONE;
    } else
      a->stable = 0;
    return 0;
  }
  uint8_t list[11], cursor;
  if (menu != 0x6160 || !(state & 0x40) || script != 1) {
    a->stable = 0;
    return 0;
  }
  if (!r(c, 0x1d03e, list, sizeof list) || !read8(r, c, 0xce84, &cursor))
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
