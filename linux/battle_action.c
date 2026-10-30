/* GPL-3.0. Port of the Android Gen 2 closed-loop input paths, with no cursor
 * writes or blind fallback. Verified normal Polished Crystal 3.2.3 only.
 * Addresses/geometry: Gen2Reader.GEN2_POLISHED_CRYSTAL, readGen2BattleInput,
 * readGen2PartyMenu; release source ram/wram{0,x}.asm. */
#include "battle_action.h"
#include <string.h>
enum {
  KEY_A = 1,
  KEY_B = 2,
  KEY_RIGHT = 16,
  KEY_LEFT = 32,
  KEY_UP = 64,
  KEY_DOWN = 128
};
static bool byte(pocket_action_read r, void *c, uint32_t addr, uint8_t *v) {
  return r(c, addr, v, 1);
}
bool pocket_battle_supported(const char *sha) {
  return sha && !strcmp(sha, "6930b48af5844d373e3c9130f26d6dd1084cf4ed");
}
bool pocket_battle_read(pocket_action_read r, void *c,
                        struct pocket_battle_menu *o) {
  *o = (struct pocket_battle_menu){.cursor = -1, .out = -1};
  uint8_t battle, m[7], cur[2], tile, count, action, out;
  if (!byte(r, c, 0x1d233, &battle))
    return false;
  if (battle != 1 && battle != 2)
    return true;
  o->state = PB_BUSY;
  if (!r(c, 0xce7c, m, 7) || !r(c, 0xce84, cur, 2))
    return false;
  int top = m[0], left = m[1], rows = m[2], cols = m[3], y = cur[0], x = cur[1];
  if (y < 1 || y > rows || x < 1 || x > cols)
    return true;
  int tx = left + (x - 1) * (m[6] & 15), ty = top + (y - 1) * (m[6] >> 4);
  if (tx >= 20 || ty >= 18)
    return true;
  if (!byte(r, c, 0xc1a0 + ty * 20 + tx, &tile))
    return false;
  if (tile != 0xf0)
    return true; /* Hollow/absent arrow means the game is busy. */
  o->rows = rows;
  if (cols == 1 && m[6] == 0x20 && top == 12 && left == 11 && rows == 3) {
    o->state = PB_SUBMENU;
    o->cursor = y - 1;
    return true;
  }
  if (cols == 1 && m[6] == 0x20 && top == 1 && left == 0) {
    if (!byte(r, c, 0x1dcce, &count) || !byte(r, c, 0x1d113, &action) ||
        !byte(r, c, 0x1d0da, &out))
      return false;
    if (count < 1 || count > 6 || rows != count + 1 ||
        (action != 0 && action != 2))
      return true;
    o->state = PB_PARTY;
    o->cursor = y == rows ? 7 : y - 1;
    o->forced = action == 2;
    o->count = count;
    o->out = out;
    return true;
  }
  if (top == 14 && left == 9 && rows == 2 && cols == 2) {
    o->state = PB_ACTION;
    o->cursor = x - 1 + 2 * (y - 1);
    return true;
  }
  uint8_t type;
  if (top == 13 && left == 5 && cols == 1 && rows >= 1 && rows <= 4) {
    if (!byte(r, c, 0x1d239, &type))
      return false;
    if (!type) {
      o->state = PB_MOVE;
      o->cursor = y - 1;
    }
  }
  return true;
}
/* Same immutable identity principle as gbMonKey: species, OT ID and DVs.
 * Retain the third Polished DV byte plus personality/form bits as additional
 * collision guards. Raw identities avoid duplicating species/form tables. */
static void identity(const uint8_t p[48], uint8_t id[8]) {
  id[0] = p[0];
  id[1] = p[6];
  id[2] = p[7];
  memcpy(id + 3, p + 17, 3);
  id[6] = p[20];
  id[7] = p[21] & 0x3f;
}
static int target_slot(struct pocket_battle_action *a, pocket_action_read r,
                       void *c, int requested) {
  uint8_t count, out, p[48];
  if (!byte(r, c, 0x1dcce, &count) || count < 1 || count > 6 ||
      !byte(r, c, 0x1d0da, &out))
    return -1;
  int found = -1;
  for (int i = 0; i < count; i++) {
    if (!r(c, 0x1dcd6 + 48 * i, p, 48))
      return -1;
    uint8_t id[8];
    identity(p, id);
    if (requested >= 0 ? i == requested : !memcmp(id, a->identity, 8)) {
      unsigned hp = (p[34] << 8) | p[35], maxHp = (p[36] << 8) | p[37];
      if (found >= 0 || !p[0] || p[0] == 255 || (p[21] & 64) || !p[31] ||
          p[31] > 100 || !hp || hp > maxHp || maxHp > 999)
        return -1;
      found = i;
      if (requested >= 0)
        memcpy(a->identity, id, 8);
    }
  }
  return found == out ? -1 : found;
}
bool pocket_battle_begin(struct pocket_battle_action *a, const char *sha,
                         enum pocket_battle_command cmd, int value,
                         pocket_action_read r, void *c) {
  if (a->result == PA_RUNNING)
    return false;
  *a = (struct pocket_battle_action){
      .result = PA_REJECTED, .command = cmd, .target = value, .lastCursor = -1};
  struct pocket_battle_menu m;
  if (!pocket_battle_supported(sha) || !pocket_battle_read(r, c, &m))
    return false;
  if (cmd == PB_CHOOSE_ACTION) {
    if (m.state != PB_ACTION || value < 0 || value > 3)
      return false;
    a->menu = PB_ACTION;
    a->pick = (int[]){0, 2, 1, 3}[value];
  } else if (cmd == PB_CHOOSE_MOVE) {
    uint8_t mon[35];
    if ((m.state != PB_ACTION && m.state != PB_MOVE) || value < 0 ||
        value > 3 || !r(c, 0xc4a5, mon, 35) || !mon[2 + value] ||
        !(mon[11 + value] & 63) || (m.state == PB_MOVE && value >= m.rows))
      return false;
    a->menu = m.state;
    a->pick = m.state == PB_ACTION ? 0 : value;
  } else if (cmd == PB_SWITCH) {
    if ((m.state != PB_ACTION && m.state != PB_PARTY &&
         m.state != PB_SUBMENU) ||
        target_slot(a, r, c, value) < 0)
      return false;
    a->menu = PB_ACTION;
    a->pick = 1;
    a->phase = m.state == PB_ACTION ? 0 : 1;
  } else if (cmd == PB_BACK) {
    if (m.state != PB_MOVE && m.state != PB_SUBMENU &&
        (m.state != PB_PARTY || m.forced))
      return false;
    a->bits = KEY_B;
    a->hold = 2;
    a->gap = 2;
    a->phase = 5;
    a->menu = m.state;
  } else
    return false;
  a->result = PA_RUNNING;
  return true;
}
static unsigned press(struct pocket_battle_action *a, unsigned bits,
                      unsigned hold, unsigned settle) {
  a->bits = bits;
  a->hold = hold - 1;
  a->gap = hold + settle;
  return bits;
}
static unsigned reject(struct pocket_battle_action *a) {
  a->result = PA_REJECTED;
  return 0;
}
unsigned pocket_battle_tick(struct pocket_battle_action *a,
                            pocket_action_read r, void *c) {
  if (a->result != PA_RUNNING)
    return 0;
  if (++a->frames > 1200 || ++a->phaseFrames > 240) {
    a->result = PA_TIMEOUT;
    return 0;
  }
  if (a->hold) {
    a->hold--;
    return a->bits;
  }
  if (a->gap) {
    a->gap--;
    return 0;
  }
  struct pocket_battle_menu m;
  if (!pocket_battle_read(r, c, &m))
    return reject(a);
  if (a->phase == 5) {
    if (m.state != (enum pocket_battle_state)a->menu) {
      a->result = PA_DONE;
      return 0;
    }
    if (a->frames - a->lastA < 8)
      return 0;
    if (++a->presses > 12)
      return reject(a);
    a->lastA = a->frames;
    return press(a, KEY_B, 2, 2);
  }
  if (a->phase == 4) { /* Android driveRunLocked: six bounded B presses. */
    if (m.state == PB_NONE || m.state == PB_ACTION || !a->presses) {
      a->result = PA_DONE;
      return 0;
    }
    a->presses--;
    return press(a, KEY_B, 1, 30);
  }
  if (m.state == PB_NONE)
    return reject(a);
  if (a->phase == 0) {
    bool waiting = m.state == (enum pocket_battle_state)a->menu;
    if (a->sentA && (!waiting || m.cursor != a->pick)) {
      a->sentA = false;
      a->presses = 0;
      a->phaseFrames = 0;
      if (a->command == PB_CHOOSE_MOVE && a->menu == PB_ACTION) {
        a->menu = PB_MOVE;
        a->pick = a->target;
        return 0;
      }
      if (a->command == PB_SWITCH) {
        a->phase = 1;
        return 0;
      }
      if (a->command == PB_CHOOSE_ACTION && a->target == 3) {
        a->phase = 4;
        a->gap = 45;
        a->presses = 6;
      } else
        a->result = PA_DONE;
      return 0; /* Accepted input, not a claim of battle outcome. */
    }
    if (!waiting)
      return 0;
    if (a->sentA && a->frames - a->lastA < 8)
      return 0;
    if (++a->presses > 12)
      return reject(a);
    if (m.cursor == a->pick) {
      a->sentA = true;
      a->lastA = a->frames;
      return press(a, KEY_A, 1, 0);
    }
    unsigned key = a->menu == PB_MOVE
                       ? (a->pick > m.cursor ? KEY_DOWN : KEY_UP)
                       : ((a->pick & 1) != (m.cursor & 1)
                              ? (a->pick & 1 ? KEY_RIGHT : KEY_LEFT)
                              : (a->pick & 2 ? KEY_DOWN : KEY_UP));
    return press(a, key, 1, 4);
  }
  int want = target_slot(a, r, c, -1);
  if (a->phase == 3 || (a->phase == 2 && (a->forced || a->switchSent))) {
    uint8_t out;
    if (!byte(r, c, 0x1d0da, &out))
      return reject(a);
    /* target_slot deliberately refuses the active slot; match its identity
     * independently here to confirm the game actually sent the chosen mon. */
    uint8_t p[48], id[8];
    if (out < 6 && r(c, 0x1dcd6 + 48 * out, p, 48)) {
      identity(p, id);
      if (!memcmp(id, a->identity, 8)) {
        a->result = PA_DONE;
        return 0;
      }
    }
    if (a->phase == 3) {
      if (m.state == PB_PARTY || m.state == PB_SUBMENU)
        return reject(a);
      return 0;
    }
  }
  if (want < 0)
    return reject(a);
  if (a->phase == 1) {
    if (m.state == PB_SUBMENU)
      return press(a, KEY_B, 2, 8);
    if (m.state != PB_PARTY)
      return 0;
    if (m.cursor != a->lastCursor) {
      a->lastCursor = m.cursor;
      if (++a->presses > 16)
        return reject(a);
    }
    if (m.cursor == want) {
      a->forced = m.forced;
      a->phase = 2;
      a->phaseFrames = 0;
      a->presses = 0;
      a->lastA = a->frames;
      return press(a, KEY_A, 2, 2);
    }
    int pos = m.cursor == 7 ? m.count : m.cursor, ring = m.count + 1;
    int down = (want - pos + ring) % ring;
    return press(a, down <= ring - down ? KEY_DOWN : KEY_UP, 2, 3);
  }
  if (a->phase == 2) {
    if (m.state == PB_SUBMENU) {
      if (m.cursor)
        return press(a, KEY_UP, 2, 3);
      if (a->switchSent && a->frames - a->lastA < 8)
        return 0;
      if (++a->presses > 12)
        return reject(a);
      a->switchSent = true;
      a->lastA = a->frames;
      return press(a, KEY_A, 2, 2);
    }
    if (m.state == PB_PARTY) {
      if (a->switchSent)
        return reject(a); /* Game refused; don't choose again. */
      if (a->frames - a->lastA >= 8) {
        a->phase = 1;
        a->phaseFrames = 0;
      }
      return 0;
    }
    if (a->forced || a->switchSent) {
      a->phase = 3;
      a->phaseFrames = 0;
    }
  }
  return 0;
}
void pocket_battle_cancel(struct pocket_battle_action *a) {
  if (a->result == PA_RUNNING)
    a->result = PA_CANCELLED;
  a->hold = a->gap = 0;
}
const char *pocket_battle_message(const struct pocket_battle_action *a) {
  switch (a->result) {
  case PA_DONE:
    return a->command == PB_SWITCH ? "Chosen Pokemon sent in"
                                   : "Battle input accepted by game";
  case PA_TIMEOUT:
    return "Battle action timed out; game control restored";
  case PA_CANCELLED:
    return "Battle action cancelled; game control restored";
  case PA_REJECTED:
    return "Battle action unavailable or refused; use game controls";
  default:
    return "Battle action in progress - B cancels";
  }
}
