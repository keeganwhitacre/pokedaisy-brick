/* Compare identical paused real-ROM work with/without idle caching.
 * Host CPU time is diagnostic, not a Brick battery-life measurement. */
#define POCKET_TEST
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
static const char *starter;
static int pass, reached;
static unsigned pausedFrame;
static uint64_t readsBefore, drawsBefore, loadedReads, loadedDraws;
static struct timespec cpuStart;
static double cpuMs[2];
static uint32_t baseline[W * H];
static char savePath[1200];
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
  if (frame == 0) assert(state(c, starter, false));
  if (frame == 2) press(SDLK_TAB);
  if (frame == 3) press(SDLK_e);
  if (frame == 10) {
    assert(companion && p->section == PC_BAG);
    assert(audio && SDL_GetAudioDeviceStatus(audio) == SDL_AUDIO_PAUSED);
    pausedFrame = c->frameCounter(c);
    readsBefore = snapshotReads;
    drawsBefore = framePresents;
    assert(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpuStart) == 0);
  }
  if (frame > 10 && frame <= 70) assert(c->frameCounter(c) == pausedFrame);
  if (frame == 70) {
    struct timespec end;
    assert(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end) == 0);
    cpuMs[pass] = (end.tv_sec - cpuStart.tv_sec) * 1000. +
                  (end.tv_nsec - cpuStart.tv_nsec) / 1.e6;
    uint64_t reads = snapshotReads - readsBefore, draws = framePresents - drawsBefore;
    printf("idle %s, 60 polls: snapshots=%llu draws=%llu process_cpu_ms=%.3f\n",
           pass ? "cached" : "uncached", (unsigned long long)reads,
           (unsigned long long)draws, cpuMs[pass]);
    assert(reads == (pass ? 0 : 60) && draws == (pass ? 0 : 60));
    if (!pass) memcpy(baseline, pixels, sizeof baseline);
    else assert(!memcmp(baseline, pixels, sizeof baseline));
    assert(capture_frame(pass ? "runtime-phase21-bag-cached.png" :
                               "runtime-phase21-bag-uncached.png", NULL));
    assert(state(c, savePath, true));
    assert(state(c, savePath, false));
    assert(c->frameCounter(c) == pausedFrame);
    loadedReads = snapshotReads;
    loadedDraws = framePresents;
  }
  if (frame == 71) {
    assert(snapshotReads == loadedReads + 1 && framePresents == loadedDraws + 1);
    assert(c->frameCounter(c) == pausedFrame);
    loadedReads = snapshotReads;
    loadedDraws = framePresents;
    assert(textures_reset());
  }
  if (frame == 72) {
    assert(framePresents == loadedDraws + 1);
    assert(snapshotReads == loadedReads + (pass ? 0 : 1));
    loadedDraws = framePresents;
    repaintEpoch++; /* SDL exposed/restored/resize uses this same invalidation. */
  }
  if (frame == 73) {
    assert(framePresents == loadedDraws + 1);
    notice("Idle expiry test");
    messageUntil = SDL_GetTicks() + 100;
    loadedDraws = framePresents;
  }
  if (frame == 90) {
    assert(SDL_GetTicks() >= messageUntil);
    assert(framePresents - loadedDraws == (pass ? 2 : 17));
    puts("PASS identical cached canvas; same-frame state-load refresh, texture/expose repaint and notice expiry; no paused core frames");
    press(SDLK_q); /* Party's selected icon must keep its original bob timing. */
  }
  if (frame == 92) {
    assert(p->section == PC_PARTY);
    drawsBefore = framePresents;
  }
  if (frame == 152) {
    uint64_t draws = framePresents - drawsBefore;
    assert(pass ? (draws >= 5 && draws <= 7) : draws == 60);
    printf("party animation over 60 polls: %llu draws\n", (unsigned long long)draws);
    puts("PASS original 167ms party animation retained without continuous redraw");
    press(SDLK_e);
  }
  if (frame >= 153 && frame <= 155) press(SDLK_e);
  if (frame == 157) {
    assert(p->section == PC_MAP && !p->placesOpen);
    drawsBefore = framePresents;
  }
  if (frame == 217) {
    uint64_t draws = framePresents - drawsBefore;
    assert(pass ? (draws >= 2 && draws <= 4) : draws == 60);
    assert(capture_frame(pass ? "runtime-phase21-map-cached.png" :
                               "runtime-phase21-map-uncached.png", NULL));
    puts("PASS original 333ms map pulse retained without continuous redraw");
    press(SDLK_F3);
  }
  if (frame == 220) {
    assert(testQuick);
    drawsBefore = framePresents;
    readsBefore = snapshotReads;
  }
  if (frame == 280) {
    assert(testQuick && c->frameCounter(c) == pausedFrame);
    assert(SDL_GetAudioDeviceStatus(audio) == SDL_AUDIO_PAUSED);
    assert(framePresents - drawsBefore == (pass ? 0 : 60));
    assert(snapshotReads - readsBefore == (pass ? 0 : 60));
    puts("PASS idle quick menu reuses its game image/overlay and avoids state-file polling");
    running = 0;
  }
}
int main(int argc, char **argv) {
  setbuf(stdout, NULL);
  assert(argc == 3);
  starter = argv[2];
  for (pass = 0; pass < 2; pass++) {
    char dir[] = "/tmp/pd-efficiency-XXXXXX";
    assert(mkdtemp(dir));
    snprintf(savePath, sizeof savePath, "%s/manual.ss", dir);
    setenv("POCKET_IDLE_CACHE", pass ? "1" : "0", 1);
    forceFresh = true;
    running = 1;
    messageUntil = 0;
    char *args[] = {"pokedaisy-pocket", "--data", dir, "--frames", "290", argv[1], NULL};
    assert(pocket_app_main(6, args) == 0 && reached == 280);
  }
  printf("idle process CPU reduction on this host: %.1f%% (not battery/runtime)\n",
         cpuMs[0] > 0 ? 100. * (1. - cpuMs[1] / cpuMs[0]) : 0.);
  return 0;
}
