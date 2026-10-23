/* Production frontend, genuine ROM state and controller-driven settings. */
#define POCKET_TEST
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
static const char *starter;
static unsigned ffCounter;
static int reached;
static void press(SDL_Keycode k) {
  SDL_Event e = {.type = SDL_KEYDOWN};
  e.key.keysym.sym = k;
  SDL_PushEvent(&e);
  e.type = SDL_KEYUP;
  SDL_PushEvent(&e);
}
static void test_frame(struct mCore *c, int *pc, bool companion, int frame,
                       struct pd_ui *u, struct pd_snapshot *s,
                       const struct pc_ui *p) {
  (void)pc; (void)u; (void)s;
  reached = frame;
  if (!frame) assert(state(c, starter, false));
  if (frame == 10) press(SDLK_ESCAPE);
  if (frame >= 11 && frame <= 19) press(SDLK_DOWN);
  if (frame == 20) press(SDLK_z);
  if (frame >= 21 && frame <= 27) press(SDLK_DOWN);
  if (frame == 28) { assert(testMenu == 6); press(SDLK_RIGHT); }
  if (frame == 29) { assert(displayOptions.ffSpeed == 3); press(SDLK_LEFT); }
  if (frame == 30) { assert(displayOptions.ffSpeed == 2); press(SDLK_LEFT); }
  if (frame == 31) {
    assert(displayOptions.ffSpeed == 4);
    assert(capture_frame("runtime-phase20-settings.png", NULL));
    press(SDLK_DOWN);
  }
  if (frame == 32) press(SDLK_z);
  if (frame == 33) { assert(!displayOptions.companionShaders); press(SDLK_DOWN); }
  if (frame == 34) press(SDLK_z);
  if (frame == 35) { assert(!displayOptions.battleBackground); press(SDLK_z); }
  if (frame == 36) { assert(displayOptions.battleBackground); press(SDLK_UP); }
  if (frame == 37) press(SDLK_z);
  if (frame == 38) { assert(displayOptions.companionShaders); press(SDLK_x); }
  if (frame == 39) press(SDLK_x);
  if (frame == 40) { assert(!companion); ffCounter = c->frameCounter(c); press(SDLK_f); }
  if (frame == 41) {
    assert(testFf && c->frameCounter(c) == ffCounter + 4);
    assert(SDL_GetAudioDeviceStatus(audio) == SDL_AUDIO_PLAYING);
    press(SDLK_f);
  }
  if (frame == 42) { assert(!testFf); press(SDLK_TAB); }
  if (frame == 43) {
    assert(companion && SDL_GetAudioDeviceStatus(audio) == SDL_AUDIO_PAUSED);
    press(SDLK_z);
  }
  if (frame == 44) {
    assert(p->summary);
    assert(capture_frame("runtime-phase20-summary.png", NULL));
    press(SDLK_RIGHT);
  }
  if (frame == 45) { assert(p->move == 1); press(SDLK_UP); }
  if (frame == 46) {
    assert(p->move == 1 && p->selected == 0);
    displayOptions.filter = POCKET_FILTER_LCD;
  }
  if (frame == 47) {
    assert(effectW == 1024 && effectH == 768 && effectTexture);
    assert(capture_frame("runtime-phase20-summary-lcd.png", NULL));
    static uint32_t before[W * H], rendered[1024 * 768], plain[1024 * 768];
    memcpy(before, pixels, sizeof before);
    present_ui(false);
    assert(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB888,
                               plain, 1024 * 4) == 0);
    present_ui(true);
    assert(!memcmp(before, pixels, sizeof before));
    assert(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB888,
                               rendered, 1024 * 4) == 0);
    assert(memcmp(rendered, plain, sizeof plain));
    displayOptions.companionShaders = false;
    present_ui(true);
    assert(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB888,
                               rendered, 1024 * 4) == 0);
    assert(!memcmp(rendered, plain, sizeof plain));
    displayOptions.companionShaders = true;
    displayOptions.filter = 0;
    displayOptions.colors = true;
    present_ui(true);
    assert(!memcmp(before, pixels, sizeof before));
    assert(SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB888,
                               rendered, 1024 * 4) == 0);
    assert(memcmp(rendered, plain, sizeof plain));
    displayOptions.colors = false;
    press(SDLK_e);
  }
  if (frame == 48) {
    assert(p->section == PC_BAG);
    assert(capture_frame("runtime-phase20-bag.png", NULL));
    struct pocket_display_options saved;
    assert(pocket_options_load(root, displayKey, &saved));
    assert(saved.ffSpeed == 4 && saved.companionShaders && saved.battleBackground);
    puts("PASS controller settings: 2x/3x/4x wrap, exact 4x core advancement, per-ROM persistence");
    puts("PASS original Summary controls retained; physical-resolution companion LCD and color effects, OFF bypass and unchanged source canvas");
    running = 0;
  }
}
int main(int argc, char **argv) {
  assert(argc == 3);
  /* Synthetic transition guards, never presented as a game capture. */
  for (unsigned i = 0; i < sizeof video / sizeof *video; i++) video[i] = 0xe0f0ff;
  battleVisible = displayOptions.battleBackground = true;
  assert(battle_background(160, 144) == 0xfff0e0);
  video[159] = 0;
  assert(!battle_background(160, 144));
  video[159] = 0xe0f0ff;
  for (int x = 1; x < 60; x++) video[x] = 0;
  assert(!battle_background(160, 144));
  for (int x = 1; x < 60; x++) video[x] = 0xe0f0ff;
  battleVisible = false;
  assert(!battle_background(160, 144));
  battleVisible = true;
  displayOptions.battleBackground = false;
  assert(!battle_background(160, 144));
  displayOptions.battleBackground = true;
  assert(!battle_background(240, 160));
  wideReady = true;
  assert(!battle_background(160, 144));
  wideReady = false;
  puts("PASS battle fill guards: transitions, mixed borders, disabled option, nonbattle and other game dimensions");
  starter = argv[2];
  char dir[] = "/tmp/pd-usability-XXXXXX";
  assert(mkdtemp(dir));
  forceFresh = true;
  char *args[] = {"pokedaisy-pocket", "--data", dir, "--frames", "70", argv[1], NULL};
  assert(pocket_app_main(6, args) == 0 && reached == 48);
  return 0;
}
