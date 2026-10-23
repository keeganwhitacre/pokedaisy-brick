/* Captured Android RAM + explicit model/fault tests adapted from
 * BattleInputControllerGbTest. Model writes never reach a running emulator. */
#include "battle_action.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char ram[32768], base[32768];
static const char *sha = "6930b48af5844d373e3c9130f26d6dd1084cf4ed";
static bool fault;
static unsigned char *addr(uint32_t a) {
  unsigned b = a >> 16, v = a & 65535;
  if (!b && v >= 0xc000 && v < 0xd000)
    return ram + v - 0xc000;
  if (b < 8 && v >= 0xd000 && v < 0xe000)
    return ram + b * 4096 + v - 0xd000;
  return NULL;
}
static bool read_mem(void *c, uint32_t a, void *out, size_t n) {
  (void)c;
  unsigned char *p = addr(a);
  if (fault || !p || (a & 4095) + n > 4096)
    return false;
  memcpy(out, p, n);
  return true;
}
static void fixture(const char *root, const char *name) {
  char path[1200];
  snprintf(path, sizeof path, "%s/%s/wramx.bin", root, name);
  FILE *f = fopen(path, "rb");
  assert(f && fread(ram, 1, sizeof ram, f) == sizeof ram);
  fclose(f);
  snprintf(path, sizeof path, "%s/%s/wram.bin", root, name);
  f = fopen(path, "rb");
  assert(f && fread(ram, 1, 4096, f) == 4096);
  fclose(f);
}
static enum pocket_battle_state mode;
static int cur, delay, chosen, usedMove, refuse;
static bool forced;
static unsigned last;
static void draw(void) {
  memset(addr(0xc1a0), 0, 360);
  unsigned char *m = addr(0xce7c), *cursor = addr(0xce84);
  memset(m, 0, 7);
  cursor[0] = cursor[1] = 1;
  *addr(0x1d233) = mode == PB_NONE ? 0 : 1;
  if (mode == PB_BUSY || mode == PB_NONE)
    return;
  m[3] = 1;
  if (mode == PB_ACTION) {
    m[0] = 14;
    m[1] = 9;
    m[2] = 2;
    m[3] = 2;
    m[6] = 0x26;
    cursor[0] = 1 + cur / 2;
    cursor[1] = 1 + cur % 2;
  }
  if (mode == PB_MOVE) {
    m[0] = 13;
    m[1] = 5;
    m[2] = 2;
    m[6] = 0x10;
    cursor[0] = 1 + cur;
  }
  if (mode == PB_PARTY) {
    m[0] = 1;
    m[2] = 3;
    m[6] = 0x20;
    cursor[0] = 1 + cur;
    *addr(0x1d113) = forced ? 2 : 0;
  }
  if (mode == PB_SUBMENU) {
    m[0] = 12;
    m[1] = 11;
    m[2] = 3;
    m[6] = 0x20;
    cursor[0] = 1 + cur;
  }
  *addr(0xc1a0 + (m[0] + (cursor[0] - 1) * (m[6] >> 4)) * 20 + m[1] +
        (cursor[1] - 1) * (m[6] & 15)) = 0xf0;
}
static void reset(enum pocket_battle_state state, bool force) {
  memcpy(ram, base, sizeof ram);
  mode = state;
  cur = 0;
  delay = 0;
  chosen = -1;
  usedMove = -1;
  refuse = 0;
  forced = force;
  last = 0;
  draw();
}
static void step(unsigned keys) {
  unsigned hit = keys & ~last;
  last = keys;
  if (delay) {
    delay--;
    return;
  }
  if (hit & 2) {
    if (mode == PB_MOVE)
      mode = PB_ACTION;
    else if (mode == PB_SUBMENU)
      mode = PB_PARTY;
    draw();
    return;
  }
  if (mode == PB_ACTION) {
    if (hit & 16)
      cur |= 1;
    if (hit & 32)
      cur &= ~1;
    if (hit & 64)
      cur &= ~2;
    if (hit & 128)
      cur |= 2;
    if (hit & 1) {
      if (cur == 0) {
        mode = PB_MOVE;
        cur = 0;
        delay = 7;
      } else if (cur == 1) {
        mode = PB_PARTY;
        cur = 0;
        delay = 8;
      } else
        mode = cur == 3 ? PB_NONE : PB_BUSY;
    } else if (hit)
      delay = 3;
  } else if (mode == PB_MOVE) {
    if (hit & 128)
      cur = (cur + 1) % 2;
    if (hit & 64)
      cur = (cur + 1) % 2;
    if (hit & 1) {
      usedMove = cur;
      mode = PB_BUSY;
    } else if (hit)
      delay = 3;
  } else if (mode == PB_PARTY || mode == PB_SUBMENU) {
    if (hit & 128)
      cur = (cur + 1) % 3;
    if (hit & 64)
      cur = (cur + 2) % 3;
    if (hit & 1) {
      if (mode == PB_PARTY && cur < 2) {
        chosen = cur;
        if (forced) {
          *addr(0x1d0da) = cur;
          mode = PB_BUSY;
        } else {
          mode = PB_SUBMENU;
          cur = 0;
          delay = 30;
        }
      } else if (mode == PB_SUBMENU && cur == 0) {
        if (refuse) {
          mode = PB_PARTY;
          cur = chosen;
        } else {
          *addr(0x1d0da) = chosen;
          mode = PB_BUSY;
        }
      }
    } else if (hit)
      delay = 3;
  }
  draw();
}
static void run(struct pocket_battle_action *a) {
  for (int i = 0; i < 1300 && a->result == PA_RUNNING; i++)
    step(pocket_battle_tick(a, read_mem, NULL));
  assert(a->result != PA_RUNNING && pocket_battle_tick(a, read_mem, NULL) == 0);
}
int main(int argc, char **argv) {
  assert(argc == 2);
  struct pocket_battle_menu m;
  const char *names[] = {
      "polished_crystal_battle_menu", "polished_crystal_battle",
      "polished_crystal_battle_party", "polished_crystal_battle_switch_menu",
      "polished_crystal_battle_faint"};
  int states[] = {PB_ACTION, PB_MOVE, PB_PARTY, PB_SUBMENU, PB_PARTY};
  for (int i = 0; i < 5; i++) {
    fixture(argv[1], names[i]);
    assert(pocket_battle_read(read_mem, NULL, &m));
    assert((int)m.state == states[i]);
    assert(m.forced == (i == 4));
  }
  fixture(argv[1], names[2]);
  memcpy(base, ram, sizeof base);
  struct pocket_battle_action a = {0};
  reset(PB_ACTION, false);
  assert(
      !pocket_battle_begin(&a, "unknown", PB_CHOOSE_ACTION, 0, read_mem, NULL));
  assert(!pocket_battle_begin(&a, sha, PB_SWITCH, 0, read_mem, NULL));
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_MOVE, 1, read_mem, NULL));
  run(&a);
  assert(a.result == PA_DONE && usedMove == 1);
  reset(PB_MOVE, false);
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_MOVE, 1, read_mem, NULL));
  run(&a);
  assert(a.result == PA_DONE && usedMove == 1);
  reset(PB_ACTION, false);
  assert(pocket_battle_begin(&a, sha, PB_SWITCH, 1, read_mem, NULL));
  run(&a);
  assert(a.result == PA_DONE && *addr(0x1d0da) == 1);
  reset(PB_PARTY, true);
  assert(pocket_battle_begin(&a, sha, PB_SWITCH, 1, read_mem, NULL));
  run(&a);
  assert(a.result == PA_DONE && *addr(0x1d0da) == 1);
  reset(PB_ACTION, false);
  refuse = 1;
  assert(pocket_battle_begin(&a, sha, PB_SWITCH, 1, read_mem, NULL));
  run(&a);
  assert(a.result == PA_REJECTED && *addr(0x1d0da) == 0);
  reset(PB_ACTION, false);
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_ACTION, 0, read_mem, NULL));
  mode = PB_BUSY;
  draw();
  run(&a);
  assert(a.result == PA_TIMEOUT);
  reset(PB_ACTION, false);
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_ACTION, 0, read_mem, NULL));
  pocket_battle_cancel(&a);
  assert(pocket_battle_tick(&a, read_mem, NULL) == 0);
  reset(PB_ACTION, false);
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_ACTION, 0, read_mem, NULL));
  fault = true;
  assert(pocket_battle_tick(&a, read_mem, NULL) == 0 &&
         a.result == PA_REJECTED);
  fault = false;
  reset(PB_PARTY, false);
  *addr(0x1d113) = 1;
  assert(pocket_battle_read(read_mem, NULL, &m) && m.state == PB_BUSY);
  reset(PB_MOVE, false);
  *addr(0xc4a5 + 12) = 0;
  assert(!pocket_battle_begin(&a, sha, PB_CHOOSE_MOVE, 1, read_mem, NULL));
  reset(PB_PARTY, false);
  assert(pocket_battle_begin(&a, sha, PB_SWITCH, 1, read_mem, NULL));
  *addr(0x1dcd6 + 48 + 6) ^= 1;
  run(&a);
  assert(a.result == PA_REJECTED);
  puts("PASS five actual upstream menu captures; closed-loop moves, dropped "
       "presses, optional/forced switching, game refusal, item-target "
       "exclusion, changed identity, PP, faults, timeout and cancellation");
  return 0;
}
