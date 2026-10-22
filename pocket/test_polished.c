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
int main(int argc, char **argv) {
  assert(argc == 2);
  test_guide();
  char hash[41];
  sha1((const unsigned char *)"abc", 3, hash);
  assert(!strcmp(hash, "a9993e364706816aba3e25717850c26c9cd0d89d"));
  sha1((const unsigned char *)"", 0, hash);
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
    assert(b.battlers[0].species == 155 && b.battlers[0].level == 5);
    if (i == 0)
      assert(b.battlers[0].hp == 13 && b.battlers[1].species == 161 &&
             b.battlers[1].hp == 13);
    if (i == 2)
      assert(b.isTrainer && b.foeCount == 1 && b.foes[0].species == 152 &&
             b.foeActive == 0);
    assert(b.dexOk);
    uint32_t canvasPixels[512 * 384];
    struct pd_canvas testCanvas;
    pd_canvas_init(&testCanvas, 512, 384, canvasPixels);
    pc_battle_draw(&testCanvas, &b, NULL, 0, 0);
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
  return 0;
}
