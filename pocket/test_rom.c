/* Optional private-ROM test. Normal controller input only: no RAM injection.
 * Never compiled into the install executable or bundled with game data. */
#define main pocket_app_main
#include "main.c"
#undef main
#include "../3ds/desktop/pd_png.h"
#include "polished.c"
#include <assert.h>
#include <mgba/core/log.h>
static void quiet_log(struct mLogger *l, int cat, enum mLogLevel level,
                      const char *fmt, va_list args) {
  (void)l;
  (void)cat;
  if (level == mLOG_ERROR) {
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
  }
}
static struct mLogger logger = {.log = quiet_log};
static void tick(struct mCore *c, unsigned keys, int frames) {
  c->setKeys(c, keys);
  while (frames-- > 0)
    c->runFrame(c);
}
int main(int argc, char **argv) {
  mLogSetDefaultLogger(&logger);
  assert(argc == 4);
  FILE *f = fopen(argv[1], "rb");
  assert(f);
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);
  unsigned char *rom = malloc(size);
  assert(rom && fread(rom, 1, size, f) == (size_t)size);
  fclose(f);
  int build = pc_identify(rom, size);
  assert(build >= 0);
  const unsigned *a = builds[build].at;
  printf("Matched supplied ROM: %s\n", builds[build].sha);
  uint32_t px[512 * 384];
  struct pd_canvas canvas;
  pd_canvas_init(&canvas, 512, 384, px);
  for (int i = 0; i < pc_dex_count(); i++) {
    int id = dexOrder[i];
    size_t e = off(a[11]) + 3u * (id - 1);
    assert(e + 3 <= (size_t)size);
    size_t p = off((rom[e] << 16) | rom[e + 1] | (rom[e + 2] << 8));
    char cat[128], t1[768], t2[768];
    assert(rom_string(rom, size, &p, build, cat, sizeof cat) && *cat);
    assert(rom_string(rom, size, &p, build, t1, sizeof t1) && *t1);
    assert(rom_string(rom, size, &p, build, t2, sizeof t2) && *t2);
    for (unsigned p = 0; p < sizeof px / sizeof *px; p++)
      px[p] = 0xffffff;
    front_pic(&canvas, 0, 0, id, rom, size, build);
    unsigned ink = 0;
    for (int j = 0; j < 112 * 512; j++)
      ink += px[j] != 0xffffff;
    assert(ink > 10);
  }
  int descriptions = 0;
  for (int item = 1; item < 0x300; item++)
    if (itemNames[item]) {
      int table = 20, index = item - 1;
      if (item > 0x200) {
        table = 21;
        index = item - 0x201;
      } else if (item > 0x100) {
        table = 22;
        index = -1;
        for (unsigned i = 0; i < sizeof tmItems / sizeof *tmItems; i++)
          if (tmItems[i] == item) {
            size_t p = off(a[24]) + i;
            assert(p < (size_t)size);
            index = rom[p] - 1;
            break;
          }
      }
      if (index < 0)
        continue;
      char text[1024];
      assert(table_text(rom, size, build, table, index, text, sizeof text) &&
             *text);
      descriptions++;
    }
  for (int i = 0; i < 3; i++) {
    uint32_t map[160 * 144];
    assert(map_image(map, rom, size, build, i));
  }
  printf("PASS %d dex texts/front pictures, %d descriptions and all 3 maps\n",
         pc_dex_count(), descriptions);
  struct VFile *vf = VFileOpen(argv[1], O_RDONLY);
  struct mCore *c = mCoreFindVF(vf);
  assert(c && c->init(c));
  mCoreInitConfig(c, NULL);
  c->setVideoBuffer(c, video, 256);
  c->setAudioBufferSize(c, 512);
  assert(c->loadROM(c, vf));
  c->reset(c);
  f = fopen(argv[2], "r");
  assert(f);
  char line[128];
  while (fgets(line, sizeof line, f)) {
    int frames;
    char button[24];
    if (sscanf(line, "wait %d", &frames) == 1) {
      assert(frames >= 0 && frames < 10000);
      tick(c, 0, frames);
    } else if (sscanf(line, "hold %23s %d", button, &frames) == 2) {
      const char *names[] = {"A",    "B",  "SELECT", "START", "RIGHT",
                             "LEFT", "UP", "DOWN",   "R",     "L"};
      int key = -1;
      for (int i = 0; i < 10; i++)
        if (!strcmp(button, names[i]))
          key = i;
      assert(key >= 0 && frames > 0 && frames < 10000);
      tick(c, 1u << key, frames);
      tick(c, 0, 1);
    } else
      assert(line[0] == '#' || line[0] == '\n');
  }
  fclose(f);
  struct pd_snapshot s;
  assert(pc_read(&s, read_memory, c));
  assert(s.partyCount == 1 && s.party[0].species == 155 &&
         s.party[0].level == 5 && s.party[0].hp > 0);
  printf("PASS actual new game: species %d Lv%d HP%d/%d EXP%lu at map %d.%d; "
         "no RAM writes\n",
         s.party[0].species, s.party[0].level, s.party[0].hp, s.party[0].maxHp,
         (unsigned long)s.party[0].experience, s.mapGroup, s.mapNum);
  assert(s.pcProgressOk && guide_badges(&s) == 0);
  assert(!guide_boss_done(&s, 0));
  // The actual starter event, read through the same generated guide flag.
  int starter = -1;
  for (unsigned i = 0; i < sizeof guideAreas / sizeof *guideAreas; i++)
    if (guideAreas[i].area == s.mapsec && guideAreas[i].kind == 4 &&
        guideAreas[i].id == 155)
      starter = (int)i;
  assert(starter >= 0 && guide_area_done(&s, starter));
  printf("PASS actual guide: %d badges, time %d, received Cyndaquil flag; next "
         "boss Falkner\n",
         guide_badges(&s), s.pcTimeOfDay);
  assert(state(c, argv[3], true));
  struct pc_ui u = {0};
  pc_ui_draw(&u, &canvas, &s, rom, size, build);
  pd_png_write("runtime-live323-party.png", 512, 384, px);
  static uint32_t still[512 * 384];
  for (int frame = 0; frame < 2; frame++) {
    pc_ui_tick(&u, frame * 167);
    pc_ui_draw(&u, &canvas, &s, rom, size, build);
    char path[80];
    snprintf(path, sizeof path, "runtime-phase11-party%d.png", frame);
    assert(pd_png_write(path, 512, 384, px));
    if (!frame)
      memcpy(still, px, sizeof still);
    else {
      int changed = 0;
      for (int y = 0; y < 384; y++)
        for (int x = 0; x < 512; x++)
          if (still[y * 512 + x] != px[y * 512 + x]) {
            changed++;
            assert(x >= 8 && x < 72 && y >= 40 && y < 128);
          }
      assert(changed > 0);
      puts("PASS selected icon animation changes only its sprite region");
    }
  }
  u.summary = true;
  pc_ui_draw(&u, &canvas, &s, rom, size, build);
  pd_png_write("runtime-live323-summary.png", 512, 384, px);
  assert(pd_png_write("runtime-phase11-summary.png", 512, 384, px));
  u.summaryPage = 1;
  pc_ui_draw(&u, &canvas, &s, rom, size, build);
  pd_png_write("runtime-live323-matchups.png", 512, 384, px);
  u.moveOpen = true;
  pc_ui_draw(&u, &canvas, &s, rom, size, build);
  pd_png_write("runtime-live323-move.png", 512, 384, px);
  for (int i = 0; i < 3; i++) {
    u = (struct pc_ui){
        .section = PC_MAP, .region = i, .mapSet = true, .mapX = 17, .mapY = 12};
    pc_ui_draw(&u, &canvas, &s, rom, size, build);
    char path[64];
    snprintf(path, sizeof path, "runtime-live323-map%d.png", i);
    pd_png_write(path, 512, 384, px);
    for (int frame = 0; frame < 2; frame++) {
      pc_ui_tick(&u, frame * 333);
      pc_ui_draw(&u, &canvas, &s, rom, size, build);
      snprintf(path, sizeof path, "runtime-phase12-map%d-pulse%d.png", i,
               frame);
      assert(pd_png_write(path, 512, 384, px));
    }
  }
  for (int filter = 0; filter < 4; filter++) {
    for (int level = 0; level < 3; level++) {
      u = (struct pc_ui){.section = PC_GUIDE,
                         .guidePage = 2,
                         .guideBossOpen = true,
                         .guideMon = 1,
                         .guideReveal = level};
      pc_ui_draw(&u, &canvas, &s, rom, size, build);
      char path[80];
      snprintf(path, sizeof path, "runtime-phase10-boss-entry%d.png", level);
      assert(pd_png_write(path, 512, 384, px));
    }
    u = (struct pc_ui){.section = PC_DEX, .dexFilter = filter};
    pc_ui_draw(&u, &canvas, &s, rom, size, build);
    char path[80];
    snprintf(path, sizeof path, "runtime-phase9-dex%d.png", filter);
    assert(pd_png_write(path, 512, 384, px));
    snprintf(path, sizeof path, "runtime-phase13-dex%d.png", filter);
    assert(pd_png_write(path, 512, 384, px));
  }
  u = (struct pc_ui){.section = PC_GUIDE,
                     .guidePage = 2,
                     .guideBossOpen = true,
                     .guideReveal = 1};
  pc_ui_draw(&u, &canvas, &s, rom, size, build);
  assert(pd_png_write("runtime-phase9-boss-compare.png", 512, 384, px));
  u = (struct pc_ui){.section = PC_DEX, .dex = 154, .detail = true};
  pc_ui_draw(&u, &canvas, &s, rom, size, build);
  pd_png_write("runtime-live323-dex.png", 512, 384, px);
  u.dexPage = 1;
  pc_ui_draw(&u, &canvas, &s, rom, size, build);
  pd_png_write("runtime-live323-dex-text.png", 512, 384, px);
  const char *guideShots[] = {"wild", "finds", "bosses", "tips", "where"};
  for (int i = 0; i < 5; i++) {
    u = (struct pc_ui){.section = PC_GUIDE, .guidePage = i};
    pc_ui_draw(&u, &canvas, &s, rom, size, build);
    char path[128];
    snprintf(path, sizeof path, "runtime-live323-guide-%s.png", guideShots[i]);
    pd_png_write(path, 512, 384, px);
  }
  u = (struct pc_ui){.section = PC_GUIDE,
                     .guidePage = 2,
                     .guideBossOpen = true,
                     .guideReveal = 2};
  pc_ui_draw(&u, &canvas, &s, rom, size, build);
  pd_png_write("runtime-live323-guide-team.png", 512, 384, px);
  assert(pc_ui_input(&u, &s, PC_DOWN) && u.guideScroll == 1);
  assert(pc_ui_input(&u, &s, PC_BACK) && !u.guideReveal);
  assert(pc_ui_input(&u, &s, PC_LEFT) && u.guidePage == 1);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.guideReveal == 1);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.guideReveal == 2);
  assert(pc_ui_input(&u, &s, PC_NEXT) && u.section == PC_PARTY);
  assert(pc_ui_input(&u, &s, PC_PREVIOUS) && u.section == PC_GUIDE);
  puts("PASS guide page/reveal/back/scroll and six-section wrap");
  int defaultWidth = pd_text_width("A");
  pc_font(rom, size, build);
  assert(pd_text_width("A") == 16);
  pd_font_set(NULL, 0);
  assert(pd_text_width("A") == defaultWidth);
  u = (struct pc_ui){.section = PC_MAP};
  map_center(&u, &s);
  assert(u.region == 0 && u.mapX == 17 && u.mapY == 12);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && u.region == 1);
  assert(pc_ui_input(&u, &s, PC_ALTERNATE) && u.region == 0);
  assert(pc_ui_input(&u, &s, PC_PLACES) && u.placesOpen);
  assert(pc_ui_input(&u, &s, PC_CONFIRM) && !u.placesOpen);
  puts("PASS map region/recenter/places and scoped ROM font");
  pc_font(rom, size, build);
  pc_item_draw(&canvas, 25, rom, size, build);
  pd_png_write("runtime-rom323-potion.png", 512, 384, px);
  pd_font_set(NULL, 0);
  for (int y = 0; y < 144; y++)
    for (int x = 0; x < 160; x++) {
      uint32_t v = video[y * 256 + x];
      px[y * 160 + x] = ((v & 255) << 16) | (v & 0xff00) | ((v >> 16) & 255);
    }
  pd_png_write("runtime-live323-game-native.png", 160, 144, px);
  c->unloadROM(c);
  mCoreConfigDeinit(&c->config);
  c->deinit(c);
  free(rom);
  return 0;
}
