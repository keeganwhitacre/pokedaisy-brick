/* Real-ROM integration. Only the hardware suspend call is substituted.
 * No host sleep/shutdown, injected RAM, personal save or ROM output. */
#define POCKET_TEST
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
static const char *starter;
static unsigned savedFrame, sleepFrame, suspendCalls;
static int pass;
static bool failSuspend;
static struct mCore *sleepCore;
static void test_frame(struct mCore *c, int *pc, bool companion, int frame,
                       struct pd_ui *u, struct pd_snapshot *s,
                       const struct pc_ui *p) {
  (void)pc;
  (void)companion;
  (void)u;
  (void)s;
  (void)p;
  if (pass == 1 && frame == 0)
    assert(state(c, starter, false));
  if (pass == 1 && frame == 2)
    savedFrame = c->frameCounter(c);
  if (pass == 2 && frame == 0)
    assert(c->frameCounter(c) == savedFrame + 1);
  if ((pass == 3 || pass == 4 || pass == 5) && frame == 0)
    assert(c->frameCounter(c) == 1);
}
static bool simulated_suspend(void) {
  suspendCalls++;
  assert(sleepCore->frameCounter(sleepCore) == sleepFrame);
  assert(!audio && !held && !pressed);
  SDL_Event e = {.type = SDL_KEYDOWN};
  e.key.keysym.sym = SDLK_z;
  SDL_PushEvent(&e);
  e.type = SDL_JOYBUTTONDOWN;
  e.jbutton.button = 1;
  SDL_PushEvent(&e);
  SDL_Delay(25);
  assert(sleepCore->frameCounter(sleepCore) == sleepFrame);
  return !failSuspend;
}
static bool fail_sync(struct VFile *v, void *p, size_t n) {
  (void)v;
  (void)p;
  (void)n;
  return false;
}
static void screenshot(const char *name) {
  present_game(160, 144);
  assert(capture_frame(name, NULL));
}
int main(int argc, char **argv) {
  assert(argc == 3);
  starter = argv[2];
  char data[] = "/tmp/pd-session-XXXXXX";
  assert(mkdtemp(data));
  root = data;
  char path[1200], battery[1200], session[1200], manual[1200];
  snprintf(path, sizeof path, "%s/saves", data);
  assert(mkdir(path, 0755) == 0);
  snprintf(battery, sizeof battery, "%s/saves/test.sav", data);
  struct VFile *rom = VFileOpen(argv[1], O_RDONLY);
  struct mCore *c = mCoreFindVF(rom);
  assert(c && c->init(c));
  mCoreInitConfig(c, NULL);
  c->setVideoBuffer(c, video, 256);
  c->setAudioBufferSize(c, 512);
  assert(c->loadROM(c, rom));
  struct pocket_save_status saveStatus;
  struct VFile *sv =
      pocket_save_wrap(VFileOpen(battery, O_CREAT | O_RDWR), &saveStatus);
  assert(sv && c->loadSave(c, sv));
  c->reset(c);
  assert(state(c, starter, false));
  FILE *f = fopen(argv[1], "rb");
  assert(f);
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  rewind(f);
  unsigned char *bytes = malloc((size_t)n);
  assert(bytes && fread(bytes, 1, n, f) == (size_t)n);
  fclose(f);
  pocket_rom_sha1(bytes, (size_t)n, displayKey);
  free(bytes);
  snprintf(session, sizeof session, "%s/recovery/%s.session", data, displayKey);
  assert(pocket_recovery_load(c, data, displayKey, battery) ==
         RECOVERY_MISSING);
  assert(pocket_recovery_save(c, data, displayKey, battery));
  /* Check the actual payload, not just the flags passed to mGBA. A future
   * serializer change must not silently reintroduce savedata restoration. */
  f = fopen(session, "rb");
  assert(f && fseek(f, 0, SEEK_END) == 0);
  long recoverySize = ftell(f) - 144;
  assert(recoverySize > 0 && fseek(f, 144, SEEK_SET) == 0);
  bytes = malloc((size_t)recoverySize);
  assert(bytes && fread(bytes, 1, recoverySize, f) == (size_t)recoverySize);
  fclose(f);
  struct VFile *payload = VFileFromConstMemory(bytes, (size_t)recoverySize);
  struct mStateExtdata ext;
  mStateExtdataInit(&ext);
  assert(payload && mCoreExtractExtdata(c, payload, &ext));
  struct mStateExtdataItem savedata;
  assert(mStateExtdataGet(&ext, EXTDATA_SAVEDATA, &savedata) && !savedata.data);
  mStateExtdataDeinit(&ext);
  payload->close(payload);
  free(bytes);
  unsigned at = c->frameCounter(c);
  c->runFrame(c);
  assert(pocket_recovery_load(c, data, displayKey, battery) == RECOVERY_LOADED);
  assert(c->frameCounter(c) == at);
  struct GB *g = c->board;
  bool (*realSync)(struct VFile *, void *, size_t) = g->sramVf->sync;
  g->sramVf->sync = fail_sync;
  assert(!pocket_recovery_save(c, data, displayKey, battery));
  g->sramVf->sync = realSync;
  assert(pocket_recovery_load(c, data, displayKey, battery) == RECOVERY_LOADED);
  f = fopen(battery, "r+b");
  assert(f);
  int old = fgetc(f);
  rewind(f);
  fputc(old ^ 1, f);
  assert(fclose(f) == 0);
  assert(pocket_recovery_load(c, data, displayKey, battery) == RECOVERY_STALE);
  assert(c->frameCounter(c) == at);
  f = fopen(battery, "r+b");
  assert(f);
  fputc(old, f);
  fclose(f);
  assert(pocket_recovery_load(c, data, displayKey, battery) == RECOVERY_LOADED);
  f = fopen(session, "r+b");
  assert(f);
  fseek(f, -1, SEEK_END);
  old = fgetc(f);
  fseek(f, -1, SEEK_END);
  fputc(old ^ 1, f);
  fclose(f);
  assert(pocket_recovery_load(c, data, displayKey, battery) ==
         RECOVERY_INVALID);
  assert(c->frameCounter(c) == at);
  assert(pocket_recovery_save(c, data, displayKey, battery));
  assert(truncate(session, 20) == 0);
  assert(pocket_recovery_load(c, data, displayKey, battery) ==
         RECOVERY_INVALID);
  assert(c->frameCounter(c) == at);
  assert(pocket_recovery_save(c, data, displayKey, battery));
  f = fopen(session, "r+b");
  assert(f && fseek(f, 8, SEEK_SET) == 0);
  fputc('x', f);
  fclose(f);
  assert(pocket_recovery_load(c, data, displayKey, battery) ==
         RECOVERY_INVALID);
  assert(c->frameCounter(c) == at);
  assert(pocket_recovery_save(c, data, displayKey, battery));
  snprintf(manual, sizeof manual, "%s/manual.ss", data);
  assert(state(c, manual, true));
  puts("PASS recovery roundtrip, failed-write preservation, "
       "corrupt/truncated/wrong-ROM rejection, changed battery and "
       "manual-state separation");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) == 0);
  window = SDL_CreateWindow("session test", 0, 0, 1024, 768, 0);
  assert(window);
  renderer = SDL_CreateRenderer(window, -1, 0);
  assert(renderer);
  assert(textures_reset());
  pd_canvas_init(&canvas, W, H, pixels);
  audio_open();
  uint32_t crc = 0;
  c->checksum(c, &crc, mCHECKSUM_CRC32);
  wideSupported = pocket_wide_configure(crc);
  assert(wideSupported);
  wideMode = 1;
  displayOptions =
      (struct pocket_display_options){.grid = 1, .autoResume = true};
  bool textExpanded = false;
  for (int i = 0; i < 80; i++) {
    c->setKeys(c, 0);
    c->runFrame(c);
    wide_tick(c, false);
    if (wideReady && pk_wide_gb_scene() == PK_WIDE_SCENE_TEXT)
      textExpanded = true;
  }
  assert(textExpanded && wideReady);
  screenshot("runtime-phase16-expanded-dialogue.png");
  displayOptions.autoZoom = true;
  c->runFrame(c);
  wide_tick(c, false);
  assert(pk_wide_gb_scene() == PK_WIDE_SCENE_TEXT && !wideReady);
  screenshot("runtime-phase16-auto-zoom.png");
  displayOptions.autoZoom = false;
  assert(state(c, manual, false));
  for (int i = 0; i < 24; i++) {
    c->runFrame(c);
    wide_tick(c, false);
  }
  assert(wideReady);
  screenshot("runtime-phase16-state-expanded.png");
  int openFrames = 0, fieldFrames = 0;
  for (int i = 0; i < 2400; i++) {
    c->setKeys(c, i % 68 < 8 ? 1 : 0);
    c->runFrame(c);
    wide_tick(c, false);
    if (wideReady && pk_wide_gb_scene() == PK_WIDE_SCENE_TEXT)
      openFrames++;
    if (wideReady && pk_wide_gb_scene() == PK_WIDE_SCENE_FIELD) {
      if (!fieldFrames)
        screenshot("runtime-phase16-expanded-field.png");
      fieldFrames++;
    }
  }
  assert(openFrames > 0 && fieldFrames > 0);
  printf("PASS expanded dialogue %d frames / field %d frames; original PPU "
         "centre asserted every expanded frame\n",
         openFrames, fieldFrames);
  sleepCore = c;
  sleepFrame = c->frameCounter(c);
  test_suspend = simulated_suspend;
  platform = (struct pocket_platform){.active = true};
  uint64_t due = 0;
  struct pocket_action action = {.result = PA_RUNNING};
  held = (1u << A) | (1u << UP);
  pressed = held;
  pocket_platform_key(&platform, true, 2000);
  assert(lifecycle(c, battery, &action, &due));
  assert(c->frameCounter(c) == sleepFrame && action.result == PA_CANCELLED);
  pocket_platform_key(&platform, false, 2100);
  assert(lifecycle(c, battery, &action, &due));
  assert(suspendCalls == 1 && audio && due && c->frameCounter(c) == sleepFrame);
  events();
  assert(!held && !pressed && !platform.sleepRequested);
  pocket_platform_key(&platform, true, SDL_GetTicks());
  assert(!platform.powerDown);
  platform.ignoreUntil = 0;
  failSuspend = true;
  pocket_platform_key(&platform, true, 3000);
  pocket_platform_key(&platform, false, 3100);
  assert(lifecycle(c, battery, &action, &due));
  assert(lifecycleFailure && audio && c->frameCounter(c) == sleepFrame);
  lifecycleFailure = failSuspend = false;
  platform.ignoreUntil = 0;
  pocket_platform_key(&platform, true, 4000);
  pocket_platform_poll(&platform, 5001);
  assert(platform.shutdownRequested && !platform.sleepRequested);
  platform = (struct pocket_platform){0};
  test_suspend = NULL;
  puts("PASS pause-before-sleep, synthetic resume, audio reopen, input "
       "release, suspend failure, cancellation and long-press detection; no "
       "hardware power invoked");
  c->unloadROM(c);
  mCoreConfigDeinit(&c->config);
  c->deinit(c);
  assert(!saveStatus.failed);
  effect_invalidate();
  SDL_DestroyTexture(texture);
  SDL_DestroyTexture(gameTexture);
  SDL_DestroyTexture(wideTexture);
  texture = gameTexture = wideTexture = NULL;
  SDL_CloseAudioDevice(audio);
  audio = 0;
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  forceFresh = true;
  pass = 1;
  running = 1;
  char *args[] = {
      "pokedaisy-pocket", "--data", data, "--frames", "3", argv[1], NULL};
  assert(pocket_app_main(6, args) == 0);
  forceFresh = false;
  pass = 2;
  running = 1;
  args[4] = "2";
  assert(pocket_app_main(6, args) == 0);
  puts("PASS actual frontend graceful exit -> relaunch restores exact game "
       "session");
  struct pocket_display_options options;
  pocket_options_load(data, displayKey, &options);
  options.autoResume = false;
  assert(pocket_options_save(data, displayKey, &options));
  pass = 3;
  running = 1;
  assert(pocket_app_main(6, args) == 0);
  options.autoResume = true;
  assert(pocket_options_save(data, displayKey, &options));
  pass = 4;
  running = 1;
  char *fresh[] = {"pokedaisy-pocket", "--data", data, "--frames", "2",
                   "--fresh",          argv[1],  NULL};
  assert(pocket_app_main(7, fresh) == 0);
  puts("PASS AUTO RESUME OFF and --fresh bypass recovery in actual frontend");
  assert(truncate(session, 20) == 0);
  forceFresh = false;
  pass = 5;
  running = 1;
  assert(pocket_app_main(6, args) == 0);
  puts("PASS corrupt recovery -> normal boot fallback in actual frontend");
  return 0;
}
