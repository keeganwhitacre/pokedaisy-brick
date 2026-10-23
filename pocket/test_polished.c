/* Decoder regression tests use upstream captured RAM, not fabricated parties.
 */
#include "../3ds/desktop/pd_png.h"
#include "polished.c"
#include <assert.h>
#include <stdlib.h>
static unsigned char ram[32768];
static bool read_fixture(void *ctx, uint32_t a, void *out, size_t n) {
  (void)ctx;
  unsigned bank = a >> 16, addr = a & 65535;
  if (!bank && addr >= 0xc000 && addr + n <= 0xd000) {
    memcpy(out, ram + addr - 0xc000, n);
    return true;
  }
  if (bank > 7 || addr < 0xd000 || addr + n > 0xe000)
    return false;
  memcpy(out, ram + bank * 4096 + addr - 0xd000, n);
  return true;
}
static void test_guide(void) {
  struct pd_snapshot s = {
      .pcProgressOk = true, .pcTimeOfDay = PC_DAY, .mapGroup = 3, .mapNum = 2};
  struct guide_encounter rows[32] = {{0}};
  assert(guide_encounters(&s, 0, rows) == 2);
  assert(rows[0].species == 19 && rows[0].min == 3 && rows[0].max == 6 &&
         rows[0].percent == 85);
  assert(rows[1].species == 69 && rows[1].percent == 15);
  s.pcTimeOfDay = PC_NITE;
  memset(rows, 0, sizeof rows);
  assert(guide_encounters(&s, 0, rows) == 3);
  assert(rows[0].species == 19 && rows[0].percent == 60);
  assert(rows[1].species == 92 && rows[1].percent == 25);
  struct guide_encounter eve[32] = {{0}};
  s.pcTimeOfDay = PC_EVE;
  assert(guide_encounters(&s, 0, eve) == 3 && !memcmp(rows, eve, sizeof rows));
  assert(guide_level(178, 0) == 4 && guide_level(178, 16) == 61);
  assert(guide_level(175, 2) == 9 && guide_level(255, 16) == 99);
  s.pcBadges = 65535;
  assert(guide_badges(&s) == 16);
  assert(guide_flag(&s, 0x1000f) && !guide_flag(&s, 0x10010));
  s.pcTrades = 2;
  assert(guide_flag(&s, 0x20001) && !guide_flag(&s, 0x20000));
  assert(!guide_boss_done(&s, 0));
  s.flags[PC_EVENT_BEAT_FALKNER / 8] |= 1u << (PC_EVENT_BEAT_FALKNER % 8);
  assert(guide_boss_done(&s, 0) && !guide_boss_done(&s, 1));
  s.flags[PC_EVENT_BEAT_ELITE_FOUR / 8] |= 1u << (PC_EVENT_BEAT_ELITE_FOUR % 8);
  assert(guide_boss_done(&s, 8) && !guide_boss_done(&s, 21));
  s.flags[PC_EVENT_BEAT_ELITE_4_WILL / 8] |=
      1u << (PC_EVENT_BEAT_ELITE_4_WILL % 8);
  assert(guide_boss_done(&s, 21));
  s.pcBadges = 0;
  assert(!guide_boss_done(&s, 21));
  s.flags[PC_EVENT_BEAT_ELITE_FOUR_AGAIN / 8] |=
      1u << (PC_EVENT_BEAT_ELITE_FOUR_AGAIN % 8);
  assert(guide_boss_done(&s, 21) && guide_boss_done(&s, 25));
  // Every generated encounter set is bounded, valid and stable across all
  // clocks.
  struct pc_ui u = {.section = PC_GUIDE, .guidePage = 3, .guideRow = 4096};
  assert(
      pc_ui_input(&u, &s, PC_CONFIRM)); // queued presses before the next draw
  assert(u.guideReveal == 1);
  int ids[1024];
  for (int category = 0; category < 3; category++) {
    int count = guide_where(category, 0, ids);
    assert(count > 0 && count < 1024);
    for (int k = 1; k < count; k++) {
      char a[128], b[128];
      guide_where_name(ids[k - 1], 0, a, sizeof a);
      guide_where_name(ids[k], 0, b, sizeof b);
      assert(strcmp(a, b) < 0);
    }
  }
  // GuideScreen.herePage: seen-only naming; hidden items show only kind;
  // owned items sort last, unique keys survive duplicate titles.
  struct guide_row model[GUIDE_ROWS];
  struct guide_entry e;
  s = (struct pd_snapshot){.pcProgressOk = true,
                           .posOk = true,
                           .dexOk = true,
                           .pcTimeOfDay = PC_DAY,
                           .mapGroup = 3,
                           .mapNum = 2,
                           .mapsec = landmark(3 * 256 + 2)};
  int count = guide_rows(&s, 0, 0, model), wild = 0;
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < i; j++)
      assert(model[i].key != model[j].key);
    guide_entry(&model[i], &s, 0, &e);
    if (model[i].kind == G_WILD) {
      wild++;
      assert(!strcmp(e.title, "???") && e.steps == 1 && *e.answer && e.icon);
      for (int d = 0; d < pc_dex_count(); d++)
        if (dexOrder[d] == model[i].encounter.species) {
          s.dexSeen[d / 8] |= 1u << (d % 8);
          s.dexCaught[d / 8] |= 1u << (d % 8);
        }
      guide_entry(&model[i], &s, 0, &e);
      assert(strcmp(e.title, "???") && !e.steps && !*e.answer && e.owned);
    }
  }
  assert(wild > 0);
  for (int i = 0; i < (int)(sizeof guideAreas / sizeof *guideAreas); i++)
    if (guideAreas[i].kind == 1 && guideAreas[i].flag) {
      struct guide_row hidden = {.kind = G_AREA, .ref = i};
      guide_entry(&hidden, &s, 0, &e);
      assert(!strcmp(e.title, "HIDDEN ITEM") && e.steps == 2 && !e.owned);
      s.flags[guideAreas[i].flag / 8] |= 1u << (guideAreas[i].flag % 8);
      guide_entry(&hidden, &s, 0, &e);
      assert(strcmp(e.title, "HIDDEN ITEM") && e.steps == 1 && e.owned);
      break;
    }
  u = (struct pc_ui){.section = PC_GUIDE};
  assert(pc_ui_input(&u, &s, PC_LEFT) && u.guidePagesFocused);
  assert(pc_ui_input(&u, &s, PC_DOWN) && u.guidePage == 2);
  assert(pc_ui_input(&u, &s, PC_RIGHT) && !u.guidePagesFocused);
  for (int i = 0; i < 4; i++)
    pc_ui_input(&u, &s, PC_ALTERNATE);
  assert(u.guidePage == 2);
  puts("PASS Guide HERE spoiler/owned/icon rules, stable keys, four-page "
       "controller navigation");
  int checked = 0, substitutions = 0;
  for (unsigned i = 0; i < sizeof guideWild / sizeof *guideWild; i++) {
    if (i && guideWild[i].map == guideWild[i - 1].map &&
        guideWild[i].method == guideWild[i - 1].method)
      continue;
    s.mapGroup = guideWild[i].map >> 8;
    s.mapNum = guideWild[i].map & 255;
    for (int time = 0; time < 4; time++) {
      s.pcTimeOfDay = time;
      memset(rows, 0, sizeof rows);
      int count = guide_encounters(&s, guideWild[i].method, rows);
      int weight = 0;
      for (int k = 0; k < count; k++) {
        assert(rows[k].species > 0 && rows[k].species < 512 &&
               names[rows[k].species]);
        assert(rows[k].min >= 1 && rows[k].max <= 100 &&
               rows[k].min <= rows[k].max);
        assert(rows[k].percent >= 0 && rows[k].percent <= 100);
        weight += rows[k].weight;
      }
      assert(!count ||
             weight == (guideWild[i].method >= 2 && guideWild[i].method <= 4
                            ? 256
                            : 100));
      checked++;
    }
  }
  for (unsigned i = 0; i < sizeof guideWild / sizeof *guideWild; i++)
    if (!guideWild[i].species) {
      s.mapGroup = guideWild[i].map >> 8;
      s.mapNum = guideWild[i].map & 255;
      s.pcTimeOfDay = PC_DAY;
      memset(rows, 0, sizeof rows);
      int count = guide_encounters(&s, guideWild[i].method, rows), day = 0;
      for (int k = 0; k < count; k++)
        day |= rows[k].species == 222;
      assert(day);
      s.pcTimeOfDay = PC_NITE;
      memset(rows, 0, sizeof rows);
      count = guide_encounters(&s, guideWild[i].method, rows);
      int night = 0;
      for (int k = 0; k < count; k++)
        night |= rows[k].species == 120;
      assert(night);
      substitutions++;
    }
  printf("PASS guide: %d map/method/clock cases, %d time-dependent fish slots, "
         "badge levels, progress and League rules\n",
         checked, substitutions);
}
static void test_battle_advice(void) {
  assert(battle_pct(9, 11, 8) == 400); // Fire vs Grass/Steel
  assert(battle_pct(12, 4, 4) == 0);   // Electric vs Ground
  assert(battle_pct(9, 10, 15) == 25); // Fire vs Water/Dragon
  assert(!strcmp(battle_verdict(0, 0), "STATUS"));
  struct pd_snapshot s = {
      .partyCount = 3, .inBattle = true, .foeCount = 2, .foeActive = 0};
  s.party[0] =
      (struct pd_mon){.species = 155, .hp = 19, .moves = {52}, .pp = {25}};
  s.party[1] =
      (struct pd_mon){.species = 158, .hp = 20, .moves = {55}, .pp = {25}};
  s.party[2] =
      (struct pd_mon){.species = 16, .hp = 15, .moves = {16}, .pp = {25}};
  struct pd_battle_mon foe = {.species = 152, .type1 = 11, .type2 = 11};
  struct battle_pick picks[6];
  int count = battle_rank(&s, &foe, 0, picks);
  assert(count == 3 && picks[0].slot == 0 && picks[1].slot == 2 &&
         picks[2].slot == 1);
  assert(picks[0].pct == 200 && picks[0].risk == 50);
  s.party[0].pp[0] = 0;
  s.party[2].hp = 0;
  assert(battle_rank(&s, &foe, 0, picks) == 1 && picks[0].slot == 1);
  s.party[1].species = 255;
  assert(!battle_rank(&s, &foe, 0, picks));
  const char *name;
  int type, power;
  battle_move(249, 0, &name, &type, &power);
  assert(!strcmp(name, "Brick Break"));
  int faithful = -1;
  for (unsigned i = 0; i < sizeof builds / sizeof *builds; i++)
    if (builds[i].faithful)
      faithful = (int)i;
  battle_move(249, faithful, &name, &type, &power);
  assert(!strcmp(name, "Rock Smash"));
  struct pc_ui u = {0};
  pc_ui_observe(&u, &s);
  assert(u.battleSeen == 1);
  s.foeActive = 1;
  pc_ui_observe(&u, &s);
  assert(u.battleSeen == 3);
  s.inBattle = false;
  pc_ui_observe(&u, &s);
  assert(!u.battleSeen && !u.wasBattle);
  s.inBattle = true;
  pc_ui_observe(&u, &s);
  assert(u.battleSeen == 2);
  u.section = PC_BATTLE;
  u.battlePage = 3;
  u.battleRow = 100;
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.battleFoe == 2 &&
         u.battlePage == 2);
  assert(pc_ui_input(&u, &s, PC_PLACES) && !u.battleFoe);
  assert(pc_ui_input(&u, &s, PC_BACK) && !u.battlePage);
  assert(!pc_ui_input(&u, &s, PC_BACK));
  puts("PASS battle matchups, upstream ranking/ties, PP/fainted/egg exclusion, "
       "faithful moves, seen/reset and navigation");
}
static void test_phase9(void) {
  struct pd_snapshot s = {.dexOk = true};
  int ids[512];
  s.dexSeen[0] = 0x7;
  s.dexCaught[0] = 0x2;
  assert(dex_entries(&s, 0, ids) == pc_dex_count());
  assert(dex_entries(&s, 1, ids) == 1 && ids[0] == 1);
  assert(dex_entries(&s, 2, ids) == 2 && ids[0] == 0 && ids[1] == 2);
  assert(dex_entries(&s, 3, ids) == pc_dex_count() - 3 && ids[0] == 3);
  struct pc_ui u = {.section = PC_DEX};
  assert(pc_ui_input(&u, &s, PC_ALTERNATE) && u.dexFilter == 1 && u.dex == 1);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.detail);
  assert(pc_ui_input(&u, &s, PC_BACK) && !u.detail && u.dexFilter == 1);
  assert(pc_ui_input(&u, &s, PC_ALTERNATE) && u.dexFilter == 2 && u.dex == 0);
  assert(pc_ui_input(&u, &s, PC_DOWN) && u.dex == 2);
  assert(pc_ui_input(&u, &s, PC_DOWN) && u.dex == 2);
  memset(s.dexSeen, 0, sizeof s.dexSeen);
  memset(s.dexCaught, 0, sizeof s.dexCaught);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && !u.detail);
  s.dexOk = false;
  assert(dex_entries(&s, 0, ids) == 0);
  struct pd_battle_mon foe = {.species = 1, .type1 = 11, .type2 = 8};
  int values[18];
  int n = battle_matchups(&foe, 0, ids, values), fire = -1;
  for (int i = 0; i < n; i++) {
    if (ids[i] == 9)
      fire = i;
    assert(values[i] > 100);
    if (i)
      assert(values[i - 1] >= values[i]);
  }
  assert(fire >= 0 && values[fire] == 400);
  n = battle_matchups(&foe, 1, ids, values);
  for (int i = 0; i < n; i++) {
    assert(values[i] > 0 && values[i] < 100);
    if (i)
      assert(values[i - 1] <= values[i]);
  }
  foe.type1 = foe.type2 = 4;
  n = battle_matchups(&foe, 2, ids, values);
  assert(n == 1 && ids[0] == 12 && values[0] == 0);
  u = (struct pc_ui){.section = PC_BATTLE, .battlePage = 3};
  assert(pc_ui_input(&u, &s, PC_ALTERNATE) && u.battlePage == 4);
  assert(pc_ui_input(&u, &s, PC_RIGHT) && u.battleType == 1);
  assert(pc_ui_input(&u, &s, PC_LEFT) && u.battleType == 0);
  assert(pc_ui_input(&u, &s, PC_ALTERNATE) && u.battlePage == 0);
  s.valid = true;
  s.partyCount = 3;
  s.party[0].species = 155;
  s.party[0].level = 5;
  s.party[1].species = 255;
  s.party[1].level = 99;
  s.party[2].species = 16;
  s.party[2].level = 10;
  assert(guide_party_best(&s) == 10);
  s.valid = false;
  assert(!guide_party_best(&s));
  s.pcProgressOk = true;
  s.pcBadges = 0;
  memset(s.flags, 0, sizeof s.flags);
  u = (struct pc_ui){.section = PC_GUIDE, .guidePage = 2};
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.guideReveal == 1);
  uint64_t firstKey = u.guideOpenKey;
  assert(pc_ui_input(&u, &s, PC_ALTERNATE) && u.guidePage == 3);
  assert(pc_ui_input(&u, &s, PC_DOWN) && u.guideOpenKey == firstKey);
  for (int i = 0; i < 3; i++)
    pc_ui_input(&u, &s, PC_ALTERNATE);
  assert(u.guidePage == 2 && u.guideReveal == 1 && u.guideOpenKey == firstKey);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.guideReveal == 2);
  assert(pc_ui_input(&u, &s, PC_DOWN) && u.guideOpenKey == firstKey);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.guideReveal == 1 &&
         u.guideOpenKey != firstKey);
  assert(pc_ui_input(&u, &s, PC_BACK) && !u.guideReveal && !u.guideOpenKey);
  assert(!pc_ui_input(&u, &s, PC_BACK));
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.guideReveal == 1);
  s.flags[PC_EVENT_BEAT_FALKNER / 8] |= 1u << (PC_EVENT_BEAT_FALKNER % 8);
  assert(pc_ui_input(&u, &s, PC_DOWN) && !u.guideReveal && !u.guideOpenKey);
  puts("PASS Guide inline boss entries: select/hint/answer, one open key, "
       "back and progress invalidation");
  puts(
      "PASS Dex four upstream filters, filtered navigation/empty guard; battle "
      "live-type groups/sorting/page wrap; Guide best level excludes eggs");
}
static void test_experience(void) {
  struct pocket_exp xp;
  const uint32_t caps[] = {1000000, 600000, 1640000, 1059860, 800000, 1250000};
  for (int rate = 0; rate < 6; rate++) {
    assert(pocket_exp_progress(rate, 100, 0, 214, &xp) &&
           xp.start == caps[rate] && xp.next == caps[rate] && xp.filled == 214);
    for (int lv = 1; lv < 100; lv++) {
      assert(pocket_exp_progress(rate, lv, 0, 214, &xp) && xp.next > xp.start &&
             xp.filled == 0);
      uint32_t start = xp.start, next = xp.next;
      assert(pocket_exp_progress(rate, lv, start, 214, &xp) && !xp.filled);
      assert(pocket_exp_progress(rate, lv, next, 214, &xp) &&
             xp.filled == 214 && !xp.toNext);
      assert(pocket_exp_progress(rate, lv, UINT32_MAX, 214, &xp) &&
             xp.filled == 214 && !xp.toNext);
    }
  }
  struct pd_mon mon = {.species = 155, .level = 5, .experience = 135};
  assert(pc_exp(&mon, 200, &xp) && xp.start == 135 && xp.next == 179 &&
         xp.toNext == 44 && !xp.filled);
  mon.experience = 157;
  assert(pc_exp(&mon, 200, &xp) && xp.filled == 100 && xp.toNext == 22);
  mon.species = 255;
  assert(!pc_exp(&mon, 200, &xp));
  assert(!pocket_exp_progress(-1, 1, 0, 20, &xp) &&
         !pocket_exp_progress(0, 0, 0, 20, &xp) &&
         !pocket_exp_progress(0, 101, 0, 20, &xp));
  puts("PASS six upstream EXP curves at every level, boundaries/clamps/MAX, "
       "Polished rate mapping and egg exclusion");
}
static void test_item_lists(void) {
  // Codec/model cases only; synthetic stacks are never runtime screenshots.
  struct pocket_item_ref original[] = {{"Zinc", 1, 3, 0, 0},
                                       {"Apple", 2, 2, 1, 0},
                                       {"Apple", 2, 2, 1, 1},
                                       {"Berry", 3, 3, 0, 1}};
  struct pocket_item_ref rows[4], scratch[4];
  memcpy(rows, original, sizeof rows);
  pocket_item_sort(rows, scratch, 4, POCKET_SORT_DEFAULT);
  assert(!memcmp(rows, original, sizeof rows));
  pocket_item_sort(rows, scratch, 4, POCKET_SORT_NAME);
  assert(rows[0].pocket == 1 && rows[0].slot == 0 && rows[1].slot == 1);
  assert(!strcmp(rows[2].name, "Berry") && !strcmp(rows[3].name, "Zinc"));
  memcpy(rows, original, sizeof rows);
  pocket_item_sort(rows, scratch, 4, POCKET_SORT_COUNT);
  assert(!strcmp(rows[0].name, "Berry") && !strcmp(rows[1].name, "Zinc"));
  assert(rows[2].slot == 0 && rows[3].slot == 1);
  pocket_item_sort(rows, scratch, 4, POCKET_SORT_TYPE);
  assert(rows[0].pocket == 0 && rows[1].pocket == 0 && rows[2].pocket == 1);
  struct pd_snapshot s = {.bagOk = true};
  struct pc_ui u = {.section = PC_BAG};
  int tabs[7];
  assert(bag_tabs(&s, &u.favorites, tabs) == 1 && tabs[0] == -1);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && !u.detail && u.pocket == -1);
  s.bag[0].count = 1;
  s.bag[1].count = 2;
  s.bag[0].items[0] = (struct pd_item){25, 1};
  s.bag[1].items[0] = (struct pd_item){1, 3};
  s.bag[1].items[1] = (struct pd_item){1, 2};
  struct pd_snapshot before = s;
  struct pocket_item_ref all[640];
  assert(bag_entries(&s, -1, 0, 0, &u.favorites, all) == 3);
  assert(all[0].id == 25 && all[1].slot == 0 && all[2].slot == 1);
  assert(bag_entries(&s, -1, 3, 0, &u.favorites, all) == 3 && all[0].quantity == 3);
  assert(bag_tabs(&s, &u.favorites, tabs) == 3 && tabs[1] == 0 && tabs[2] == 1);
  assert(pc_ui_input(&u, &s, PC_DOWN) && u.item == 1 && u.detail);
  assert(pc_ui_input(&u, &s, PC_BACK) && !u.detail);
  assert(pc_ui_input(&u, &s, PC_RIGHT) && u.pocket == 0 && !u.item);
  assert(pc_ui_input(&u, &s, PC_RIGHT) && u.pocket == 1);
  assert(pc_ui_input(&u, &s, PC_RIGHT) && u.pocket == -1);
  assert(!memcmp(&s, &before, sizeof s));
  // Android DEFAULT stable favorites partition; both stacks of an id share
  // the star, other sort modes keep their original comparator.
  u.item = 1;
  u.detail = true;
  assert(pc_ui_input(&u, &s, PC_FAVORITE));
  assert(u.favoritesDirty && pocket_favorite_has(&u.favorites, 1));
  assert(bag_entries(&s, BAG_ALL, 0, 0, &u.favorites, all) == 3);
  assert(all[0].id == 1 && all[0].slot == 0 && all[1].slot == 1 && all[2].id == 25);
  assert(bag_tabs(&s, &u.favorites, tabs) == 4 && tabs[1] == BAG_FAVORITES);
  assert(pc_ui_input(&u, &s, PC_RIGHT) && u.pocket == BAG_FAVORITES);
  assert(bag_entries(&s, BAG_FAVORITES, 0, 0, &u.favorites, all) == 2);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.detail);
  assert(pc_ui_input(&u, &s, PC_FAVORITE));
  assert(u.pocket == BAG_ALL && !pocket_favorite_has(&u.favorites, 1));
  assert(bag_tabs(&s, &u.favorites, tabs) == 3);
  // A favorite not currently held persists, but does not create a dead tab.
  pocket_favorite_toggle(&u.favorites, 65535);
  assert(bag_tabs(&s, &u.favorites, tabs) == 3);
  assert(!memcmp(&s, &before, sizeof s));
  for (int p = 0; p < PD_POCKET_COUNT; p++) {
    s.bag[p].count = PD_POCKET_SLOTS;
    for (int i = 0; i < PD_POCKET_SLOTS; i++)
      s.bag[p].items[i] = (struct pd_item){1, i + 1};
  }
  assert(bag_count(&s, -1, &u.favorites) == 640);
  assert(bag_entries(&s, -1, 3, 0, &u.favorites, all) == 640);
  assert(all[0].quantity == 128 && all[639].quantity == 1);
  s.bag[0].count++;
  assert(bag_entries(&s, -1, 0, 0, &u.favorites, all) == 512);
  puts("PASS read-only item models: stable duplicate stacks, all/present "
       "pockets, "
       "name/type/count sorting, boundaries, capacity and nested controller "
       "Back");
}
static void test_pixel_widgets(void) {
  // Unknown, fainted, tiny positive, full and corrupt/out-of-range health.
  assert(!pocket_hp_fill(0, 100, 146));
  assert(!pocket_hp_fill(10, 0, 146));
  assert(!pocket_hp_fill(-1, 100, 146));
  assert(pocket_hp_fill(1, 999, 146) == 2);
  assert(pocket_hp_fill(50, 100, 146) == 72);
  assert(pocket_hp_fill(999, 100, 146) == 146);
  assert(pocket_hp_fill(INT32_MAX, INT32_MAX, 146) == 146);
  uint32_t px[512 * 384];
  struct pd_canvas c;
  pd_canvas_init(&c, 512, 384, px);
  const int hp[] = {100, 51, 50, 21, 20, 1, 0};
  const uint32_t colors[] = {0x00bd00, 0x00bd00, 0xffbd00, 0xffbd00,
                             0xff0000, 0xff0000, 0xffffff};
  for (int i = 0; i < 7; i++) {
    pd_fill(&c, 0, 0, 512, 384, 0x123456);
    pocket_hp_bar(&c, 100, 60, 154, hp[i], 100, &polishedHp);
    assert(px[64 * 512 + 104] == colors[i]);
    assert(px[59 * 512 + 104] == 0x123456);
    assert(px[74 * 512 + 104] == 0x123456);
  }
  // The cursor's center remains transparent; all corners pulse outward.
  for (int big = 0; big < 2; big++) {
    pd_fill(&c, 0, 0, 512, 384, 0x123456);
    pocket_map_cursor(&c, 100, 100, 116, 116, 2, big, 0xffffff);
    assert(px[108 * 512 + 108] == 0x123456);
    int outside = big ? 94 : 96;
    assert(px[outside * 512 + outside] == 0xffffff);
    assert(px[(outside - 1) * 512 + outside] == 0x123456);
  }
  // Full descriptions must not paint outside the viewport or change clips.
  pd_font_set(NULL, 0);
  pd_fill(&c, 0, 0, 512, 384, 0x123456);
  struct pd_canvas before = c;
  int page = 0;
  const char *text = "One two three four five six seven eight nine ten eleven "
                     "twelve thirteen.";
  int pages = pocket_text_page(&c, 20, 50, 100, 18, 2, text, &page, 0xffffff,
                               PD_NO_SHADOW);
  assert(pages > 1 && !memcmp(&before, &c, sizeof c));
  for (int y = 0; y < 384; y++)
    for (int x = 0; x < 512; x++)
      if (x < 20 || x >= 120 || y < 50 || y >= 86)
        assert(px[y * 512 + x] == 0x123456);
  page = pages;
  assert(pocket_text_page(&c, 20, 50, 100, 18, 2, text, &page, 0xffffff,
                          PD_NO_SHADOW) == pages &&
         !page);
  pd_fill(&c, 0, 0, 512, 384, 0x000000);
  pd_clip(&c, 10, 10, 4, 4);
  pocket_fade_rect(&c, -10, -10, 100, 100, 0xffffff, 45);
  assert(px[10 * 512 + 10] == 0x8c8c8c && px[9 * 512 + 10] == 0);
  pd_unclip(&c);
  pd_fill(&c, 0, 0, 512, 384, 0);
  pd_clip(&c, 10, 10, 15, 4);
  pocket_row_divider(&c, 10, 10, 25, 2, 0xffffff);
  assert(px[10 * 512 + 10] == 0xffffff && px[10 * 512 + 21] == 0xffffff);
  assert(px[10 * 512 + 22] == 0 && px[10 * 512 + 25] == 0 &&
         px[14 * 512 + 10] == 0);
  pd_unclip(&c);
  assert(pd_text_wrap(NULL, 0, 0, 100, 20, 128, "A\nB", 0, PD_NO_SHADOW) == 2);
  assert(pd_text_wrap(NULL, 0, 0, 100, 20, 128, "A\n\nB", 0, PD_NO_SHADOW) ==
         3);
  assert(pd_font_glyph(0xB7)->codepoint == 0xB7);
  puts("PASS shared pixel widgets: HP boundaries/colors, map pulse, clipped "
       "full-text pagination and dimmed sprites");
}
int main(int argc, char **argv) {
  assert(argc == 2 || argc == 3);
  test_battle_advice();
  test_phase9();
  test_experience();
  test_pixel_widgets();
  test_item_lists();
  unsigned char *cart = NULL;
  size_t romSize = 0;
  int build = 0;
  if (argc == 3) {
    FILE *f = fopen(argv[2], "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    romSize = ftell(f);
    rewind(f);
    cart = malloc(romSize);
    assert(cart && fread(cart, 1, romSize, f) == romSize);
    fclose(f);
    build = pc_identify(cart, romSize);
    assert(build >= 0);
  }
  test_guide();
  char hash[41];
  pocket_rom_sha1((const unsigned char *)"abc", 3, hash);
  assert(!strcmp(hash, "a9993e364706816aba3e25717850c26c9cd0d89d"));
  pocket_rom_sha1((const unsigned char *)"", 0, hash);
  assert(!strcmp(hash, "da39a3ee5e6b4b0d3255bfef95601890afd80709"));
  char flowed[128] = "anger-\ned fire-\ntype Poke-\nmon";
  rom_flow(flowed);
  assert(!strcmp(flowed, "angered fire-type Pokemon"));
  assert(sizeof builds / sizeof *builds == 16);
  assert(!strcmp(moves[249].name, "Brick Break"));
  assert(!strcmp(faithfulMoves[249].name, "Rock Smash"));
  unsigned char decoded[512];
  unsigned char literal[] = {2, 1, 2, 3, 255};
  assert(lz(literal, sizeof literal, 0, decoded, sizeof decoded) == 3);
  assert(decoded[2] == 3);
  unsigned char repeat[] = {0x20, 0xab, 255};
  assert(lz(repeat, sizeof repeat, 0, decoded, sizeof decoded) == 2);
  assert(decoded[1] == 0xab);
  unsigned char invalid[] = {0x80, 0xff, 255};
  assert(lz(invalid, sizeof invalid, 0, decoded, sizeof decoded) == -1);
  const char *fixtures[] = {"polished_crystal", "polished_crystal_3_2_0",
                            "polished_crystal_3_2_1", "polished_crystal_3_2_2",
                            "polished_crystal_faithful"};
  for (unsigned i = 0; i < sizeof fixtures / sizeof *fixtures; i++) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s/wramx.bin", argv[1], fixtures[i]);
    FILE *f = fopen(path, "rb");
    assert(f);
    size_t n = fread(ram, 1, sizeof ram, f);
    fclose(f);
    assert(n == sizeof ram);
    struct pd_snapshot s;
    assert(pc_read(&s, read_fixture, NULL));
    assert(s.partyCount == 2);
    assert(s.pcProgressOk);
    assert(s.party[0].species == 155 && s.party[0].level == 5 &&
           s.party[0].hp == 13 && s.party[0].maxHp == 19);
    assert(s.party[1].species == 16 && s.party[1].hp == 3 &&
           s.party[1].maxHp == 15 && s.party[1].gender == PD_GENDER_FEMALE);
    assert(s.bagOk && s.money == 3000);
    assert(s.party[0].heldItem == 0x44);
    assert(s.bag[PD_POCKET_ITEMS].count == 1 &&
           s.bag[PD_POCKET_ITEMS].items[0].id == 25 &&
           s.bag[PD_POCKET_ITEMS].items[0].quantity == 1);
    assert(s.bag[PD_POCKET_BALLS].count == 1 &&
           s.bag[PD_POCKET_BALLS].items[0].id == 1 &&
           s.bag[PD_POCKET_BALLS].items[0].quantity == 3);
    printf("PASS %s: party, held item, bag and money\n", fixtures[i]);
    if (i == 0) {
      uint32_t px[512 * 384];
      struct pd_canvas c;
      pd_canvas_init(&c, 512, 384, px);
      pc_draw(&c, &s, 0, false, NULL, 0, -1);
      pd_png_write("polished-fixture.png", 512, 384, px);
      if (cart) {
        struct pc_ui bag = {.section = PC_BAG};
        pc_ui_draw(&bag, &c, &s, cart, romSize, build);
        assert(bag.detail && bag.pocket == -1);
        assert(pd_png_write("runtime-phase13-bag-inline-fixture.png", 512, 384,
                            px));
        assert(pc_ui_input(&bag, &s, PC_CONFIRM) && !bag.detail);
        pc_ui_draw(&bag, &c, &s, cart, romSize, build);
        assert(
            pd_png_write("runtime-phase13-bag-list-fixture.png", 512, 384, px));
        int keyPocket = bag.bagKeyPocket, keySlot = bag.bagKeySlot;
        assert(pc_ui_input(&bag, &s, PC_ALTERNATE) && bag.bagSort == 1);
        pc_ui_draw(&bag, &c, &s, cart, romSize, build);
        assert(bag.bagKeyPocket == keyPocket && bag.bagKeySlot == keySlot);
        assert(pd_png_write("runtime-phase13-bag-sorted-fixture.png", 512, 384,
                            px));
        pc_ui_input(&bag, &s, PC_CONFIRM);
        pc_ui_input(&bag, &s, PC_FAVORITE);
        pc_ui_draw(&bag, &c, &s, cart, romSize, build);
        assert(pd_png_write("runtime-phase17-bag-favorites.png", 512, 384, px));
        pc_ui_input(&bag, &s, PC_RIGHT);
        assert(bag.pocket == BAG_FAVORITES);
        pc_ui_draw(&bag, &c, &s, cart, romSize, build);
        assert(pd_png_write("runtime-phase17-bag-favorites-tab.png", 512, 384, px));
        const int pages[] = {0, 2, 3, 4};
        const char *labels[] = {"here", "boss", "tips", "where"};
        for (int page = 0; page < 4; page++) {
          struct pc_ui guide = {.section = PC_GUIDE, .guidePage = pages[page]};
          pc_ui_draw(&guide, &c, &s, cart, romSize, build);
          assert(guide.guideScroll == 0); // Opening a page retains its heading.
          char path[128];
          snprintf(path, sizeof path, "runtime-phase14-guide-%s-fixture.png",
                   labels[page]);
          assert(pd_png_write(path, 512, 384, px));
          if (page == 1)
            for (int step = 1; step <= 2; step++) {
              assert(pc_ui_input(&guide, &s, PC_CONFIRM));
              pc_ui_draw(&guide, &c, &s, cart, romSize, build);
              snprintf(path, sizeof path,
                       "runtime-phase14-guide-boss-%s-fixture.png",
                       step == 1 ? "hint" : "answer");
              assert(pd_png_write(path, 512, 384, px));
            }
        }
        struct pc_ui dexUi = {.section = PC_DEX};
        pc_ui_draw(&dexUi, &c, &s, cart, romSize, build);
        assert(pd_png_write("runtime-phase13-dex-fixture.png", 512, 384, px));
      }
    }
  }
  const char *battleFixtures[] = {"polished_crystal_battle",
                                  "polished_crystal_battle_faint",
                                  "polished_crystal_trainer"};
  for (int i = 0; i < 3; i++) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s/wramx.bin", argv[1], battleFixtures[i]);
    FILE *f = fopen(path, "rb");
    assert(f);
    assert(fread(ram, 1, sizeof ram, f) == sizeof ram);
    fclose(f);
    struct pd_snapshot b;
    assert(pc_read(&b, read_fixture, NULL));
    assert(b.inBattle);
    unsigned addr = (PC_RAM_curBattleMon >> 16) * 4096 +
                    (PC_RAM_curBattleMon & 65535) - 0xd000;
    unsigned char old = ram[addr];
    ram[addr] = b.partyCount - 1;
    struct pd_snapshot selected;
    assert(pc_read(&selected, read_fixture, NULL) &&
           selected.partyActive == b.partyCount - 1);
    ram[addr] = 255;
    assert(pc_read(&selected, read_fixture, NULL) &&
           selected.partyActive == -1);
    ram[addr] = old;
    assert(pc_read(&b, read_fixture, NULL));
    assert(b.battlers[0].species == 155 && b.battlers[0].level == 5);
    if (i == 0)
      assert(b.battlers[0].hp == 13 && b.battlers[1].species == 161 &&
             b.battlers[1].hp == 13);
    if (i == 2)
      assert(b.isTrainer && b.foeCount == 1 && b.foes[0].species == 152 &&
             b.foeActive == 0);
    assert(b.dexOk);
    unsigned char battleRaw[35];
    assert(read_fixture(NULL, PC_RAM_enemyMon, battleRaw, sizeof battleRaw));
    assert(b.battlers[1].heldItem == battleRaw[1]);
    for (int k = 0; k < 5; k++)
      assert(b.battlers[1].stats[k] == be16(battleRaw + 23 + 2 * k));
    uint32_t canvasPixels[512 * 384];
    struct pd_canvas testCanvas;
    pd_canvas_init(&testCanvas, 512, 384, canvasPixels);
    pc_battle_draw(&testCanvas, &b, NULL, 0, 0);
    struct pc_ui ui = {.section = PC_BATTLE};
    pc_ui_observe(&ui, &b);
    for (int page = 0; page < 5; page++) {
      ui.battlePage = page;
      pc_ui_draw(&ui, &testCanvas, &b, cart, romSize, build);
      if (cart) {
        char path[128];
        snprintf(path, sizeof path, "runtime-phase6-%s-page%d-fixture.png",
                 battleFixtures[i], page);
        pd_png_write(path, 512, 384, canvasPixels);
      }
    }
    if (cart && i == 2)
      for (int group = 0; group < 3; group++) {
        ui.battlePage = 4;
        ui.battleType = group;
        ui.battleRow = 0;
        pc_ui_draw(&ui, &testCanvas, &b, cart, romSize, build);
        char path[128];
        snprintf(path, sizeof path, "runtime-phase9-types%d-fixture.png",
                 group);
        assert(pd_png_write(path, 512, 384, canvasPixels));
      }
    if (cart && i == 2) {
      for (int page = 0; page < 4; page++) {
        ui.battlePage = page;
        pc_ui_draw(&ui, &testCanvas, &b, cart, romSize, build);
        char path[96];
        snprintf(path, sizeof path, "runtime-phase12-battle%d-fixture.png",
                 page);
        assert(pd_png_write(path, 512, 384, canvasPixels));
      }
      ui.battlePage = 1;
      pc_ui_draw(&ui, &testCanvas, &b, cart, romSize, build);
      assert(pd_png_write("runtime-phase11-suggestions-fixture.png", 512, 384,
                          canvasPixels));
    }
    pc_dex_draw(&testCanvas, &b, 288, false, NULL, 0, 0);
    pc_dex_draw(&testCanvas, &b, 154, true, NULL, 0, 0);
    pc_item_draw(&testCanvas, 25, NULL, 0, 0);
    printf("PASS %s: live battle decoder and dex flags\n", battleFixtures[i]);
  }
  // The dex maps species to its own order, including the two skipped IDs.
  memset(ram + 0x1e7b, 0, 37);
  memset(ram + 0x1ead, 0, 37);
  ram[0x1e7b + (dexOrder[288] - 1) / 8] |= 1u << ((dexOrder[288] - 1) % 8);
  struct pd_snapshot dex;
  assert(pc_read(&dex, read_fixture, NULL));
  assert(pd_dex_caught(&dex, 289) && pd_dex_seen(&dex, 289));
  assert(!pd_dex_seen(&dex, 288));
  // Synthetic codec inputs test malformed text, not fictional runtime parties.
  unsigned char *rom = calloc(1, 0x200000);
  assert(rom);
  size_t pos = 0;
  char text[80];
  rom[0] = 0x80;
  rom[1] = 0x81;
  rom[2] = 0x53;
  assert(rom_string(rom, 0x200000, &pos, 0, text, sizeof text) &&
         !strcmp(text, "AB") && pos == 3);
  size_t tree = off(builds[0].at[25]);
  rom[tree] = 0x80;
  rom[tree + 1] = 1;
  rom[tree + 2] = 0x81;
  rom[tree + 3] = 0xf2;
  rom[0] = 0x5d;
  rom[1] = 0x58;
  pos = 0;
  assert(rom_string(rom, 0x200000, &pos, 0, text, sizeof text) &&
         !strcmp(text, "AB") && pos == 2);
  size_t pic = off(builds[0].at[13]), sizeAt = off(builds[0].at[14]);
  size_t pal = off(builds[0].at[2]) + 8;
  rom[sizeAt] = 0x10;
  rom[pic] = 0x7e;
  rom[pic + 1] = 0;
  rom[pic + 2] = 0x40;
  size_t tiles = off(0x7e4000);
  rom[tiles] = 15;
  for (int y = 0; y < 8; y++) {
    rom[tiles + 1 + 2 * y] = 255;
    rom[tiles + 2 + 2 * y] = 0;
  }
  rom[tiles + 17] = 255;
  rom[pal] = 31;
  uint32_t pixels[512 * 384] = {0};
  struct pd_canvas canvas;
  pd_canvas_init(&canvas, 512, 384, pixels);
  front_pic(&canvas, 0, 0, 1, rom, 0x200000, 0);
  assert(pixels[96 * 512 + 48] ==
         0xff0000); // column-major tile placement / palette / 2x
  size_t words = off(builds[0].at[20]);
  rom[words] = 0;
  rom[words + 1] = 0x70;
  size_t itemText = off((builds[0].at[20] & 0xff0000) | 0x7000);
  rom[itemText] = 0x80;
  rom[itemText + 1] = 0x81;
  rom[itemText + 2] = 0x53;
  assert(table_text(rom, 0x200000, 0, 20, 0, text, sizeof text) &&
         !strcmp(text, "AB"));
  assert(!table_text(rom, 1, 0, 20, 0, text, sizeof text));
  puts("PASS ROM front-picture tile placement/palette and description pointer "
       "bounds");
  memset(rom + tree, 0, 4);
  pos = 0;
  assert(!rom_string(rom, 0x200000, &pos, 0, text, sizeof text));
  pos = 0;
  assert(!rom_string(rom, 1, &pos, 0, text, sizeof text));
  free(rom);
  puts("PASS national dex mapping, caught implies seen, plain/Huffman ROM text "
       "and corrupt-tree rejection");
  // Restore the original inventory fixture before boundary mutations below.
  char restore[1024];
  snprintf(restore, sizeof restore, "%s/polished_crystal/wramx.bin", argv[1]);
  FILE *restoreFile = fopen(restore, "rb");
  assert(restoreFile);
  assert(fread(ram, 1, sizeof ram, restoreFile) == sizeof ram);
  fclose(restoreFile);
  ram[0x193f] = 1;
  ram[0x1940] = 0x44;
  ram[0x1941] = 2;
  ram[0x1942] = 255;
  ram[0x1800] = 1;
  ram[0x1801] = 0;
  ram[0x17f5] = 1;
  struct pd_snapshot bagTest;
  assert(pc_read(&bagTest, read_fixture, NULL));
  assert(bagTest.bagOk);
  assert(bagTest.bag[PD_POCKET_BERRIES].items[0].id == 0x44 &&
         bagTest.bag[PD_POCKET_BERRIES].items[0].quantity == 2);
  assert(bagTest.bag[PD_POCKET_KEY].items[0].id == 0x201);
  assert(bagTest.bag[PD_POCKET_TMHM].items[0].id == 0x101);
  ram[0x1827] = 75;
  for (int i = 0; i < 75; i++) {
    ram[0x1828 + 2 * i] = 25;
    ram[0x1829 + 2 * i] = 1;
  }
  ram[0x18be] = 255;
  ram[0x18bf] = 37;
  for (int i = 0; i < 37; i++) {
    ram[0x18c0 + 2 * i] = 0x44;
    ram[0x18c1 + 2 * i] = 2;
  }
  ram[0x190a] = 255;
  assert(pc_read(&bagTest, read_fixture, NULL) && bagTest.bagOk);
  assert(bagTest.bag[PD_POCKET_ITEMS].count == 112);
  puts(
      "PASS bag capacity 112, key/TM/berry pockets, malformed-count rejection");
  ram[0x1827] = 76;
  assert(pc_read(&bagTest, read_fixture, NULL));
  assert(!bagTest.bagOk && bagTest.bag[PD_POCKET_ITEMS].count == 0);
  ram[0x1cd6 + 21] |= 64;
  struct pd_snapshot egg;
  assert(pc_read(&egg, read_fixture, NULL) && egg.party[0].species == 255);
  struct pc_ui eggUi = {.summary = true};
  assert(pc_ui_input(&eggUi, &egg, PC_CONFIRM) && !eggUi.moveOpen);
  puts("PASS egg summary conceals moves");
  ram[0x1cce] = 7;
  struct pd_snapshot s;
  assert(!pc_read(&s, read_fixture, NULL));
  puts("PASS invalid party count and SHA-1 known vector");
  free(cart);
  return 0;
}
