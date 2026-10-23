/* GPL-3.0. Real ROM frontend navigation, no game-memory writes. */
#define POCKET_TEST
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
static const char *starter;
static char dataRoot[] = "/tmp/pd-favorites-ui-XXXXXX";
static int pass;
static unsigned paused;
static uint16_t favoriteId;
static void press(SDL_Keycode key) {
  SDL_Event e = {.type = SDL_KEYDOWN};
  e.key.keysym.sym = key;
  SDL_PushEvent(&e);
  e.type = SDL_KEYUP;
  SDL_PushEvent(&e);
}
static void test_frame(struct mCore *c, int *pc, bool companion, int frame,
                       struct pd_ui *ui, struct pd_snapshot *s,
                       const struct pc_ui *u) {
  (void)ui;
  assert(*pc >= 0);
  if (frame == 0) {
    assert(state(c, starter, false));
    press(SDLK_TAB);
  }
  if (frame == 1) {
    assert(companion);
    paused = c->frameCounter(c);
    press(SDLK_e);
  }
  if (frame == 2) {
    fprintf(stderr,
            "Bag observation: companion=%d section=%d ready=%d detail=%d "
            "counts=%d,%d,%d,%d,%d\n",
            companion, u->section, s->bagOk, u->detail, s->bag[0].count,
            s->bag[1].count, s->bag[2].count, s->bag[3].count, s->bag[4].count);
    assert(companion && u->section == PC_BAG && s->bagOk && u->detail);
    favoriteId = s->bag[u->bagKeyPocket].items[u->bagKeySlot].id;
    assert(favoriteId);
    assert(pocket_favorite_has(&u->favorites, favoriteId) == (pass == 1));
    capture_frame(pass ? "runtime-phase17-favorites-relaunch.png"
                       : "runtime-phase17-favorites-before.png",
                  NULL);
    press(SDLK_TAB); // In the description: toggle favorite, keep companion
                     // paused.
  }
  if (frame == 3) {
    assert(companion && u->detail && c->frameCounter(c) == paused);
    assert(pocket_favorite_has(&u->favorites, favoriteId) == (pass == 0));
    struct pocket_favorites disk;
    assert(pocket_favorites_load(dataRoot, displayKey, &disk));
    assert(pocket_favorite_has(&disk, favoriteId) == (pass == 0));
    assert(!u->favoritesDirty);
    capture_frame("runtime-phase17-favorites-controller.png", NULL);
    press(SDLK_z); // Close description.
  }
  if (frame == 4) {
    assert(companion && !u->detail);
    press(SDLK_TAB);
  }
  if (frame == 5) {
    assert(!companion);
  }
  if (frame == 7) {
    assert(c->frameCounter(c) > paused);
  }
}
int main(int argc, char **argv) {
  assert(argc == 3);
  starter = argv[2];
  assert(mkdtemp(dataRoot));
  forceFresh = true;
  char *args[] = {
      "pokedaisy-pocket", "--data", dataRoot, "--frames", "8", argv[1], NULL};
  for (pass = 0; pass < 2; pass++) {
    running = 1;
    assert(pocket_app_main(6, args) == 0);
  }
  puts("PASS actual ROM: X favorites keeps companion paused, persists across "
       "relaunch, unstars, and outside description X resumes game");
}
