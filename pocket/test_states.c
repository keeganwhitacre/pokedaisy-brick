/* Real-ROM manual state/preview and one-ROM launch-to-quit workflow. */
#define POCKET_TEST
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
static const char *starter;
static int pass, reached;
static unsigned closeFrame;
static void press(SDL_Keycode key) {
  SDL_Event e = {.type = SDL_KEYDOWN};
  e.key.keysym.sym = key;
  SDL_PushEvent(&e);
  e.type = SDL_KEYUP;
  SDL_PushEvent(&e);
}
static void test_frame(struct mCore *c, int *pc, bool companion, int frame,
                       struct pd_ui *u, struct pd_snapshot *s,
                       const struct pc_ui *p) {
  (void)pc;
  (void)u;
  (void)s;
  (void)p;
  if (pass == 1) {
    assert(frame == 0 && c->frameCounter(c) == closeFrame + 1);
    SDL_Event e = {.type = SDL_QUIT};
    SDL_PushEvent(&e);
    return;
  }
  if (frame == 0)
    assert(state(c, starter, false));
  if (frame < 10)
    return; // allow the loaded legacy game to render normally
  reached = frame;
  frame -= 10;
  switch (frame) {
  case 0:
    assert(!companion);
    press(SDLK_ESCAPE);
    break;
  case 1:
    assert(testMenu == 1);
    press(SDLK_DOWN);
    break;
  case 2:
    press(SDLK_DOWN);
    break;
  case 3:
    press(SDLK_z);
    break;
  case 4:
    assert(testMenu == 7);
    capture_frame("runtime-phase19-slots-empty.png", NULL);
    press(SDLK_z);
    break;
  case 5:
    assert(!companion);
    press(SDLK_ESCAPE);
    break;
  case 6:
    press(SDLK_z);
    break;
  case 7:
    assert(testMenu == 7);
    capture_frame("runtime-phase19-slots-preview.png", NULL);
    press(SDLK_RIGHT);
    break;
  case 8:
    press(SDLK_z);
    break;
  case 9:
    assert(!companion);
    press(SDLK_ESCAPE);
    break;
  case 10:
    press(SDLK_z);
    break;
  case 11:
    assert(testMenu == 7);
    press(SDLK_f);
    break;
  case 12:
    press(SDLK_z);
    break;
  case 13:
    assert(testMenu == 7);
    capture_frame("runtime-phase19-slots-confirm.png", NULL);
    press(SDLK_x);
    break;
  case 14:
    press(SDLK_z);
    break;
  case 15:
    press(SDLK_z);
    break;
  case 16:
    assert(!companion);
    press(SDLK_ESCAPE);
    break;
  case 17:
    press(SDLK_DOWN);
    break;
  case 18:
    press(SDLK_DOWN);
    break;
  case 19:
    press(SDLK_DOWN);
    break;
  case 20:
    closeFrame = c->frameCounter(c);
    press(SDLK_z);
    break;
  default:
    assert(!"Close with automatic resume must exit directly");
  }
}
int main(int argc, char **argv) {
  assert(argc == 3);
  starter = argv[2];
  struct VFile *rom = VFileOpen(argv[1], O_RDONLY);
  struct mCore *c = mCoreFindVF(rom);
  assert(c && c->init(c));
  mCoreInitConfig(c, NULL);
  c->setVideoBuffer(c, video, 256);
  assert(c->loadROM(c, rom));
  c->reset(c);
  assert(state(c, starter, false));
  struct GB *g = c->board;
  pocket_rom_sha1(g->memory.rom, g->memory.romSize, displayKey);
  for (int i = 0; i < 10; i++)
    c->runFrame(c);
  char data[] = "/tmp/pd-slots-XXXXXX", quick[1200], path[1200];
  assert(mkdtemp(data));
  snprintf(quick, sizeof quick, "%s/manual.ss", data);
  struct pocket_slot *info = malloc(sizeof *info);
  assert(info);
  for (int slot = 0; slot < 10; slot++) {
    assert(pocket_slot_path(quick, slot, path));
    pocket_slot_read(c, path, displayKey, info);
    assert(info->status == SLOT_EMPTY);
    unsigned at = c->frameCounter(c);
    assert(pocket_slot_save(c, path, displayKey));
    pocket_slot_read(c, path, displayKey, info);
    assert(info->status == SLOT_READY && info->width == 160 &&
           info->height == 144);
    const uint32_t *raw = (const uint32_t *)video;
    for (int y = 0; y < 144; y++)
      for (int x = 0; x < 160; x++) {
        uint32_t n = raw[y * 256 + x];
        assert(info->preview[y * 160 + x] ==
               (((n & 255) << 16) | (n & 0xff00) | ((n >> 16) & 255)));
      }
    c->runFrame(c);
    assert(pocket_slot_load(c, path, displayKey) && c->frameCounter(c) == at);
    pocket_slot_read(c, path, "0000000000000000000000000000000000000000", info);
    assert(info->status == SLOT_INVALID);
  }
  assert(!pocket_slot_path(quick, 10, path));
  assert(!pocket_slot_save(c, "/dev/null/unwritable", displayKey));
  char foreign[1200];
  snprintf(foreign, sizeof foreign, "%s/with-savedata.ss", data);
  struct VFile *embedded = VFileOpen(foreign, O_CREAT | O_TRUNC | O_RDWR);
  assert(embedded);
  assert(mCoreSaveStateNamed(c, embedded, SAVESTATE_RTC | SAVESTATE_SAVEDATA));
  embedded->close(embedded);
  pocket_slot_read(c, foreign, displayKey, info);
  assert(info->status == SLOT_INVALID);
  assert(!pocket_slot_load(c, foreign, displayKey));
  pocket_slot_path(quick, 1, path);
  FILE *f = fopen(path, "r+b");
  assert(f);
  fseek(f, 100, SEEK_SET);
  int byte = fgetc(f);
  fseek(f, 100, SEEK_SET);
  fputc(byte ^ 1, f);
  fclose(f);
  unsigned at = c->frameCounter(c);
  pocket_slot_read(c, path, displayKey, info);
  assert(info->status == SLOT_INVALID);
  assert(!pocket_slot_load(c, path, displayKey) && c->frameCounter(c) == at);
  assert(truncate(path, 20) == 0);
  pocket_slot_read(c, path, displayKey, info);
  assert(info->status == SLOT_INVALID);
  pocket_slot_read(c, starter, displayKey, info);
  assert(info->status == SLOT_LEGACY);
  assert(pocket_slot_load(c, starter, displayKey));
  free(info);
  c->unloadROM(c);
  mCoreConfigDeinit(&c->config);
  c->deinit(c);
  puts("PASS ten slots, RGB previews, timestamp, state roundtrip, "
       "wrong-ROM/hash/corruption rejection, legacy state and write failure");
  char dir[1200], link[1200];
  snprintf(dir, sizeof dir, "%s/roms", data);
  assert(!mkdir(dir, 0755));
  snprintf(link, sizeof link, "%s/roms/only.gbc", data);
  assert(!symlink(argv[1], link));
  char *args[] = {"pokedaisy-pocket", "--data", data, "--frames", "32", NULL};
  forceFresh = false;
  running = 1;
  assert(!pocket_app_main(5, args) && reached == 30);
  pass = 1;
  running = 1;
  assert(!pocket_app_main(5, args));
  puts("PASS single-ROM Tools autolaunch, save/preview/load/cancel, automatic "
       "gameplay return, one-step close and recovery relaunch");
}
