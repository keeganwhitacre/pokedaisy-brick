/* Genuine starter -> Lyra battle by ordinary buttons; no fixture/RAM writes.
 * Companion controls then drive the real game through the production host. */
#define POCKET_TEST
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
#include <mgba/core/log.h>
static const char *battleState = "/tmp/pd-real-battle.ss";
static int stage, oldPP;
static bool cancelTest, hotkeyPassed;
static unsigned pauseCounter;
static void send_key(int sym) {
  SDL_Event e = {.type = SDL_KEYDOWN};
  e.key.keysym.sym = sym;
  SDL_PushEvent(&e);
  e.type = SDL_KEYUP;
  SDL_PushEvent(&e);
}
static void test_frame(struct mCore *c, int *pc, bool companion, int frame,
                       struct pd_ui *u, struct pd_snapshot *s,
                       const struct pc_ui *p) {
  (void)pc;
  (void)u;
  if (cancelTest) {
    if (frame == 0)
      assert(state(c, battleState, false));
    if (frame == 1)
      send_key(SDLK_TAB);
    if (frame == 2 || frame == 3)
      send_key(SDLK_e);
    if (frame == 4)
      send_key(SDLK_RETURN);
    if (frame == 5)
      send_key(SDLK_z);
    if (frame == 6) {
      SDL_Event e = {.type = SDL_KEYDOWN};
      e.key.keysym.sym = SDLK_RETURN;
      SDL_PushEvent(&e);
      e.key.keysym.sym = SDLK_BACKSPACE;
      SDL_PushEvent(&e);
      pauseCounter = c->frameCounter(c);
    }
    if (frame == 7) {
      assert(companion && testMenu == 1 && c->frameCounter(c) == pauseCounter);
      assert(c->getKeys(c) == 0);
      assert(state(c, battleState, false));
      SDL_Event e = {.type = SDL_KEYUP};
      e.key.keysym.sym = SDLK_RETURN;
      SDL_PushEvent(&e);
      e.key.keysym.sym = SDLK_BACKSPACE;
      SDL_PushEvent(&e);
    }
    if (frame == 8)
      send_key(SDLK_x);
    if (frame == 25) {
      struct pocket_battle_menu m;
      assert(pocket_battle_read(read_memory, c, &m));
      assert(!companion && m.state == PB_ACTION && c->getKeys(c) == 0);
      hotkeyPassed = true;
      running = 0;
    }
    return;
  }
  if (frame == 0)
    assert(state(c, battleState, false));
  if (frame == 1)
    send_key(SDLK_TAB);
  if (frame == 2 || frame == 3)
    send_key(SDLK_e);
  if (frame == 4)
    send_key(SDLK_RETURN);
  if (frame == 5) {
    assert(companion && p->battleControls && p->battleMenu.state == PB_ACTION);
    assert(capture_frame("runtime-phase17-actions.png", NULL));
    send_key(SDLK_RIGHT); /* PkMn's companion party picker. */
  }
  if (frame == 6)
    send_key(SDLK_z);
  if (frame == 7) {
    assert(p->battleParty && companion);
    assert(capture_frame("runtime-phase17-party.png", NULL));
    send_key(SDLK_z);
  }
  if (frame == 8) {
    assert(p->battleParty && companion);
    send_key(SDLK_x);
  } /* Active mon cannot be sent in again. */
  if (frame == 9) {
    assert(!p->battleParty && p->battleControls);
    send_key(SDLK_LEFT);
  }
  if (frame == 10) {
    send_key(SDLK_z);
    stage = 1;
  }
  if (stage == 1 && companion && p->battleControls &&
      p->battleMenu.state == PB_MOVE) {
    oldPP = s->battlers[0].pp[1];
    assert(oldPP > 0);
    assert(capture_frame("runtime-phase17-moves.png", NULL));
    send_key(SDLK_RIGHT);
    stage = 2;
  } else if (stage == 2) {
    send_key(SDLK_z);
    stage = 3;
  } else if (stage == 3) {
    if (companion && p->battleControls && p->battleMenu.state == PB_ACTION) {
      assert(s->battlers[0].pp[1] == oldPP - 1);
      assert(capture_frame("runtime-phase17-turn-complete.png", NULL));
      stage = 4;
      running = 0;
    } else if (!companion && (frame % 60 == 30 || frame % 60 == 32)) {
      /* Gameplay consumes held keys, unlike companion focus's edge events. */
      SDL_Event e = {.type = frame % 60 == 30 ? SDL_KEYDOWN : SDL_KEYUP};
      e.key.keysym.sym = SDLK_z;
      SDL_PushEvent(&e);
    }
  }
}
static void quiet(struct mLogger *l, int cat, enum mLogLevel level,
                  const char *fmt, va_list ap) {
  (void)l;
  (void)cat;
  if (level == mLOG_ERROR) {
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
  }
}
static struct mLogger logger = {.log = quiet};
static void frames(struct mCore *c, unsigned keys, int n) {
  c->setKeys(c, keys);
  for (int i = 0; i < n; i++)
    c->runFrame(c);
}
static void drive(struct mCore *c, struct pocket_battle_action *a) {
  for (int i = 0; i < 1300 && a->result == PA_RUNNING; i++) {
    c->setKeys(c, pocket_battle_tick(a, read_memory, c));
    c->runFrame(c);
  }
  c->setKeys(c, 0);
  assert(a->result == PA_DONE);
}
static bool wait_menu(struct mCore *c, int stateWanted, int limit) {
  for (int i = 0; i < limit; i++) {
    struct pocket_battle_menu m;
    assert(pocket_battle_read(read_memory, c, &m));
    if ((int)m.state == stateWanted)
      return true;
    frames(c, 0, 1);
  }
  return false;
}
int main(int argc, char **argv) {
  setbuf(stdout, NULL);
  assert(argc == 3);
  mLogSetDefaultLogger(&logger);
  struct VFile *vf = VFileOpen(argv[1], O_RDONLY);
  struct mCore *c = mCoreFindVF(vf);
  assert(c && c->init(c));
  mCoreInitConfig(c, NULL);
  c->setVideoBuffer(c, video, 256);
  c->setAudioBufferSize(c, 512);
  assert(c->loadROM(c, vf));
  assert(c->loadSave(c, VFileMemChunk(NULL, 0)));
  c->reset(c);
  assert(state(c, argv[2], false));
  const char *sha = "6930b48af5844d373e3c9130f26d6dd1084cf4ed";
  struct pocket_action probe = {0};
  bool freeField = false;
  for (int i = 0; i < 100 && !freeField; i++) {
    frames(c, 1, 8);
    frames(c, 0, 60);
    freeField = pocket_action_begin(&probe, sha, read_memory, c);
    pocket_action_cancel(&probe);
  }
  assert(freeField);
  /* wYCoord (1:DCAE), X follows: generated/verified by gen_polished_wide.py.
   * ElmsLab.asm's scene-6 coord event at (4,6) starts Lyra's normal battle. */
  uint8_t xy[2];
  for (int i = 0; i < 20; i++) {
    assert(read_memory(c, 0x1dcae, xy, 2));
    fprintf(stderr, "walk lab x=%u y=%u\n", xy[1], xy[0]);
    if (xy[1] == 4)
      break;
    frames(c, xy[1] > 4 ? 32 : 16, 1);
    frames(c, 0, 17);
  }
  for (int i = 0; i < 20; i++) {
    assert(read_memory(c, 0x1dcae, xy, 2));
    if (xy[0] >= 6)
      break;
    frames(c, 128, 1);
    frames(c, 0, 17);
  }
  bool ready = false;
  for (int i = 0; i < 150; i++) {
    struct pocket_battle_menu m;
    assert(pocket_battle_read(read_memory, c, &m));
    if (m.state == PB_ACTION) {
      ready = true;
      break;
    }
    frames(c, 1, 3);
    frames(c, 0, 57);
  }
  assert(ready);
  assert(state(c, battleState, true));
  struct pd_snapshot s;
  assert(pc_read(&s, read_memory, c) && s.inBattle && s.isTrainer);
  printf("PASS actual Lyra battle reached by ordinary movement/dialogue: foe "
         "%d, party %d\n",
         s.battlers[1].species, s.partyCount);
  struct pocket_battle_action a = {0};
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_ACTION, 0, read_memory, c));
  drive(c, &a);
  assert(wait_menu(c, PB_MOVE, 300));
  assert(pocket_battle_begin(&a, sha, PB_BACK, 0, read_memory, c));
  drive(c, &a);
  assert(wait_menu(c, PB_ACTION, 300));
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_ACTION, 0, read_memory, c));
  drive(c, &a);
  assert(wait_menu(c, PB_MOVE, 300));
  assert(pc_read(&s, read_memory, c));
  int before = s.battlers[0].pp[1], first = s.battlers[0].pp[0];
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_MOVE, 1, read_memory, c));
  drive(c, &a);
  ready = false;
  for (int i = 0; i < 150; i++) {
    struct pocket_battle_menu m;
    assert(pocket_battle_read(read_memory, c, &m));
    if (m.state == PB_ACTION) {
      ready = true;
      break;
    }
    frames(c, 1, 3);
    frames(c, 0, 57);
  }
  assert(ready && pc_read(&s, read_memory, c));
  assert(s.battlers[0].pp[1] == before - 1 && s.battlers[0].pp[0] == first);
  puts("PASS actual second move selected and used once; correct PP consumed, "
       "first move unchanged");
  assert(state(c, battleState, false));
  frames(c, 0, 20);
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_ACTION, 1, read_memory, c));
  drive(c, &a);
  frames(c, 0, 120);
  uint8_t ptr[2];
  assert(read_memory(c, 0xce61, ptr, 2));
  unsigned menu = ptr[0] | (ptr[1] << 8);
  fprintf(stderr, "battle Bag header: %04x\n", menu);
  assert(menu == 0x4791 || menu == 0x47a9 || menu == 0x47c1 || menu == 0x47d9 ||
         menu == 0x47f1);
  puts("PASS actual battle Bag opened through original controller behavior");
  assert(state(c, battleState, false));
  frames(c, 0, 20);
  assert(pocket_battle_begin(&a, sha, PB_CHOOSE_ACTION, 3, read_memory, c));
  drive(c, &a);
  assert(wait_menu(c, PB_ACTION, 300));
  assert(pc_read(&s, read_memory, c) && s.inBattle && s.isTrainer);
  puts("PASS original move BACK and trainer RUN refusal; battle rules "
       "preserved");
  if (getenv("POCKET_TEST_ITEMS_STATE")) {
    // Obtain an actual item through the game's normal progression. Elm's
    // source has scene 5's aide gift at (4,8), after the can-lose Lyra battle.
    assert(state(c, battleState, false));
    bool field = false;
    struct pocket_action probe = {0};
    for (int i = 0; i < 1500 && !field; i++) {
      frames(c, 1, 3); frames(c, 0, 57);
      field = pocket_action_begin(&probe, sha, read_memory, c);
      pocket_action_cancel(&probe);
    }
    assert(field && pc_read(&s, read_memory, c) && !s.inBattle);
    for (int i = 0; i < 30; i++) {
      assert(read_memory(c, 0x1dcae, xy, 2));
      if (xy[0] >= 8) break;
      frames(c, 128, 1); frames(c, 0, 17);
    }
    bool obtained = false;
    for (int i = 0; i < 150 && !obtained; i++) {
      frames(c, 1, 3); frames(c, 0, 57);
      assert(pc_read(&s, read_memory, c));
      obtained = s.bagOk && s.bag[PD_POCKET_ITEMS].count > 0 &&
                 pocket_action_begin(&probe, sha, read_memory, c);
      pocket_action_cancel(&probe);
    }
    assert(obtained && s.bag[PD_POCKET_ITEMS].items[0].id == 25);
    assert(state(c, getenv("POCKET_TEST_ITEMS_STATE"), true));
    puts("PASS normal Lyra battle completion and original aide Potion gift; real inventory state exported for favorites frontend test");
  }
  c->unloadROM(c);
  mCoreConfigDeinit(&c->config);
  c->deinit(c);
  char data[] = "/tmp/pd-battle-ui-XXXXXX";
  assert(mkdtemp(data));
  forceFresh = true;
  running = 1;
  char *args[] = {
      "pokedaisy-pocket", "--data", data, "--frames", "1200", argv[1], NULL};
  assert(pocket_app_main(6, args) == 0 && stage == 4);
  puts("PASS actual companion Fight -> second move -> game turn -> automatic "
       "return, with actual PP change");
  cancelTest = true;
  running = 1;
  args[4] = "32";
  assert(pocket_app_main(6, args) == 0 && hotkeyPassed);
  puts("PASS Start+Select cancels battle action, pauses with released keys, "
       "and state load/resume cannot restart stale input");
  return 0;
}
