/* Real-ROM integration: no injected RAM, no user's save file, no ROM output. */
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
static void run_keys(struct mCore *c, int keys, int frames) {
  c->setKeys(c, keys);
  for (int i = 0; i < frames; i++)
    c->runFrame(c);
}
static void options_test(void) {
  char dir[] = "/tmp/pd-options-XXXXXX";
  assert(mkdtemp(dir));
  const char *sha = "6930b48af5844d373e3c9130f26d6dd1084cf4ed";
  struct pocket_display_options o = {3, 2, true, false}, p;
  assert(pocket_options_save(dir, sha, &o));
  assert(pocket_options_load(dir, sha, &p));
  assert(p.filter == 3 && p.grid == 2 && p.colors && !p.stretch);
  o.stretch = true;
  assert(!pocket_options_save(dir, sha, &o));
  assert(!pocket_options_save(dir, "../../bad", &o));
  char path[256];
  snprintf(path, sizeof path, "%s/display/%s.fx", dir, sha);
  FILE *f = fopen(path, "w");
  assert(f);
  fputs("pocket-fx=1\nfilter=3\n", f);
  fclose(f);
  assert(!pocket_options_load(dir, sha, &p) && p.filter == 0 && !p.colors);
  unlink(path);
  snprintf(path, sizeof path, "%s/display", dir);
  rmdir(path);
  rmdir(dir);
  /* Pixel-centre shader equations at 5x GB, 4x GBA and expanded sizes. */
  int dims[][4] = {{800, 720, 160, 144},
                   {960, 640, 240, 160},
                   {816, 608, 204, 152},
                   {1024, 768, 256, 192}};
  for (unsigned d = 0; d < 4; d++)
    for (int filter = 1; filter <= 3; filter += 2)
      for (int grid = 0; grid < 3; grid++)
        for (int y = 0; y < dims[d][1]; y += 7)
          for (int x = 0; x < dims[d][0]; x += 7) {
            double u = (x + .5) * dims[d][2] / dims[d][0],
                   v = (y + .5) * dims[d][3] / dims[d][1];
            int alpha = filter == 1 ? (((int)floor(u * 3) % 3 == 0 ||
                                        (int)floor(v * 3) % 3 == 0)
                                           ? (int[]){51, 89, 128}[grid]
                                           : 0)
                                    : ((int)floor(v * 2) % 2 == 0 ? 128 : 0);
            assert(pocket_filter_mask(filter, grid, x, y, dims[d][0],
                                      dims[d][1], dims[d][2],
                                      dims[d][3]) == ((uint32_t)alpha << 24));
          }
  pocket_color_lut(colorLut);
  assert(colorLut[0] == 0);
  /* Reference vectors evaluated from upstream GLSL's column-major matrix. */
  assert(colorLut[31] == 0xea536a);
  assert(colorLut[31 << 5] == 0x73d358);
  assert(colorLut[31 << 10] == 0x0088db);
  assert(colorLut[32767] == 0xfffffb);
  assert(pocket_color_apply(colorLut, 0x8442c6) == 0x686ea5);
  assert(pocket_color_apply(colorLut, 0xffffff) == colorLut[32767]);
  puts("PASS shader equations, color LUT, per-ROM persistence and corrupt "
       "settings");
}
int main(int argc, char **argv) {
  options_test();
  if (argc == 1)
    return 0;
  assert(argc == 3);
  struct VFile *vf = VFileOpen(argv[1], O_RDONLY);
  struct mCore *c = mCoreFindVF(vf);
  assert(c && c->init(c));
  mCoreInitConfig(c, NULL);
  c->setVideoBuffer(c, video, 256);
  c->setAudioBufferSize(c, 512);
  assert(c->loadROM(c, vf));
  c->reset(c);
  assert(state(c, argv[2], false));
  struct pocket_action a = {0};
  const char *sha = "6930b48af5844d373e3c9130f26d6dd1084cf4ed";
  /* The earlier normal-input starter snapshot is still in Elm's dialogue. */
  assert(!pocket_action_begin(&a, sha, read_memory, c));
  bool freeField = false;
  for (int i = 0; i < 100; i++) {
    run_keys(c, 1, 8);
    run_keys(c, 0, 60);
    if (pocket_action_begin(&a, sha, read_memory, c)) {
      pocket_action_cancel(&a);
      freeField = true;
      break;
    }
  }
  assert(freeField);
  assert(!pocket_action_begin(&a, "unsupported", read_memory, c));
  assert(pocket_action_begin(&a, sha, read_memory, c));
  for (int i = 0; i < 400 && a.result == PA_RUNNING; i++) {
    unsigned k = pocket_action_tick(&a, read_memory, c);
    assert(k == 0 ||
           k == 8); /* Unchanging field observation never permits A. */
  }
  assert(a.result == PA_TIMEOUT && pocket_action_tick(&a, read_memory, c) == 0);

  assert(pocket_action_begin(&a, sha, read_memory, c));
  pocket_action_cancel(&a);
  assert(a.result == PA_CANCELLED &&
         pocket_action_tick(&a, read_memory, c) == 0);
  assert(pocket_action_begin(&a, sha, read_memory, c));
  while (a.result == PA_RUNNING) {
    c->setKeys(c, pocket_action_tick(&a, read_memory, c));
    c->runFrame(c);
  }
  c->setKeys(c, 0);
  fprintf(stderr, "action: %s after %u frames\n", pocket_action_message(&a),
          a.frames);
  assert(a.result == PA_DONE);
  assert(!pocket_action_begin(&a, sha, read_memory, c)); // Already in Bag.
  puts("PASS real-ROM controller-only Bag dispatch; dialogue/unknown ROM/menu "
       "rejection and cancellation");
  assert(SDL_Init(SDL_INIT_VIDEO) == 0);
  window = SDL_CreateWindow("test", 0, 0, 1024, 768, 0);
  assert(window);
  renderer = SDL_CreateRenderer(window, -1, 0);
  assert(renderer);
  gameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING, 256, 256);
  assert(gameTexture);
  const char *names[] = {"none", "lcd", "scanlines", "colors", "stretch"};
  for (int mode = 0; mode < 5; mode++) {
    displayOptions = (struct pocket_display_options){.filter = mode == 1   ? 1
                                                               : mode == 2 ? 3
                                                                           : 0,
                                                     .grid = 1,
                                                     .colors = mode == 3,
                                                     .stretch = mode == 4};
    effect_invalidate();
    present_game(160, 144);
    char path[100];
    snprintf(path, sizeof path, "runtime-phase15-%s.png", names[mode]);
    assert(capture_frame(path, NULL));
    uint64_t start = SDL_GetPerformanceCounter();
    for (int i = 0; i < 180; i++)
      present_game(160, 144);
    printf("render %s: %.3f ms/frame (SDL %s, warm cache; no emulator/audio)\n",
           names[mode],
           1000. * (SDL_GetPerformanceCounter() - start) /
               SDL_GetPerformanceFrequency() / 180,
           SDL_GetCurrentVideoDriver());
  }
  effect_invalidate();
  SDL_DestroyTexture(gameTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  c->deinit(c);
  return 0;
}
