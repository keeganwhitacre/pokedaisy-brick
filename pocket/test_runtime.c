#define POCKET_TEST
#define main pocket_app_main
#include "main.c"
#undef main
#include <assert.h>
static unsigned lastCounter, pausedCounter;
static bool gbaTest;
static bool realRomTest, quickTest, wideTest, wideReopen;
static char quickRoot[] = "/tmp/pd-quick-test-XXXXXX";
static const char *quickRom;
static unsigned quickPause, quickSaved;
static void send_key(SDL_Keycode k) {
  SDL_Event e = {.type = SDL_KEYDOWN};
  e.key.keysym.sym = k;
  SDL_PushEvent(&e);
  e.type = SDL_KEYUP;
  SDL_PushEvent(&e);
}
static void send_button(int button) {
  SDL_Event e = {.type = SDL_JOYBUTTONDOWN};
  e.jbutton.button = button;
  SDL_PushEvent(&e);
  e.type = SDL_JOYBUTTONUP;
  SDL_PushEvent(&e);
}
static void quick_test_frame(struct mCore *c, bool companion, int frame) {
  if (frame == 0 && getenv("POCKET_TEST_STATE"))
    assert(state(c, getenv("POCKET_TEST_STATE"), false));
  if (frame == 5) {
    quickPause = c->frameCounter(c);
    send_button(12);
  }
  if (frame == 6) {
    assert(testQuick && c->frameCounter(c) == quickPause);
    capture_frame("runtime-phase7-quick.png", NULL);
    send_key(SDLK_RIGHT);
  }
  if (frame == 7) {
    assert(testQuickSelected == 1);
    send_key(SDLK_z);
  }
  if (frame == 8) {
    assert(testQuick && !testMenu);
    send_key(SDLK_DOWN);
  }
  if (frame == 9) {
    assert(testQuickSelected == 2);
    send_key(SDLK_z);
  }
  if (frame == 10) {
    assert(!testQuick && testFf && c->frameCounter(c) == quickPause);
    send_key(SDLK_F3);
  }
  if (frame == 11) {
    assert(testQuick);
    send_key(SDLK_UP);
  }
  if (frame == 12)
    send_key(SDLK_z);
  if (frame == 13) {
    assert(!testQuick && companion && testMenu == 5);
    send_key(SDLK_DOWN);
    send_key(SDLK_z);
    quickSaved = c->frameCounter(c);
  }
  if (frame == 14) {
    assert(testMenu == 1);
    char path[1400];
    const char *name = strrchr(quickRom, '/');
    name = name ? name + 1 : quickRom;
    snprintf(path, sizeof path, "%s/states/%s.ss", quickRoot, name);
    assert(access(path, F_OK) == 0);
    send_key(SDLK_x);
  }
  if (frame == 17) {
    assert(!companion && c->frameCounter(c) > quickSaved);
    send_button(12);
  }
  if (frame == 18) {
    assert(testQuick);
    send_key(SDLK_RIGHT);
  }
  if (frame == 19)
    send_key(SDLK_z);
  if (frame == 20) {
    assert(testMenu == 2);
    send_key(SDLK_x);
  }
  if (frame == 21) {
    assert(testMenu == 1);
    send_key(SDLK_x);
  }
  if (frame == 24)
    send_key(SDLK_F3);
  if (frame == 25) {
    assert(testQuick);
    send_key(SDLK_RIGHT);
  }
  if (frame == 26)
    send_key(SDLK_z);
  if (frame == 27) {
    assert(testMenu == 2);
    send_key(SDLK_DOWN);
    send_key(SDLK_z);
  }
  if (frame == 28) {
    assert(c->frameCounter(c) == quickSaved && testMenu == 1);
    send_key(SDLK_x);
  }
  if (frame == 31)
    send_button(12);
  if (frame == 32) {
    assert(testQuick);
    send_key(SDLK_LEFT);
  }
  if (frame == 33)
    send_key(SDLK_z);
  if (frame == 34) {
    assert(companion && !testQuick && !testMenu);
    send_key(SDLK_TAB);
  }
  if (frame == 37)
    send_key(SDLK_F3);
  if (frame == 38) {
    assert(testQuick);
    capture_frame("runtime-phase7-quick-ff.png", NULL);
    send_key(SDLK_F3);
  }
  if (frame == 39)
    assert(!testQuick);
  if (frame >= 40 && frame < 60) {
    if (frame % 2 == 0) {
      assert(!testQuick);
      quickPause = c->frameCounter(c);
    } else
      assert(testQuick && c->frameCounter(c) == quickPause);
    send_button(12);
  }
  if (frame == 61)
    send_key(SDLK_ESCAPE);
  if (frame == 62) {
    assert(companion && testMenu == 1);
    send_key(SDLK_UP);
    send_key(SDLK_z);
  }
  if (frame == 63) {
    assert(testQuick);
    send_key(SDLK_x);
  }
  if (frame == 64) {
    assert(!testQuick && testMenu == 1);
    send_key(SDLK_x);
  }
  if (frame == 65) {
    assert(!companion);
  }
  if (frame == 66)
    puts("PASS quick menu: raw L3/keyboard, D-pad, disabled load, FF, "
         "save/load confirmation/cancel/round-trip, companion, pause/resume "
         "and repeated toggles");
}
static void wide_test_frame(struct mCore *c, bool companion, int frame) {
  if (wideReopen) {
    assert(wideMode == 1);
    if (!frame)
      puts("PASS remembered display after emulator teardown/reopen");
    return;
  }
  if (frame == 0) {
    assert(wideSupported);
    assert(state(c, getenv("POCKET_TEST_STATE"), false));
  }
  if (frame < 1800 && (frame % 30 == 0 || frame % 30 == 10)) {
    SDL_Event e = {.type = frame % 30 == 0 ? SDL_KEYDOWN : SDL_KEYUP};
    e.key.keysym.sym = SDLK_z;
    SDL_PushEvent(&e);
  }
  if (frame == 1801)
    send_key(SDLK_F4);
  if (frame == 1825) {
    printf("expand ready=%d score=%d scene=%d\n", wideReady,
           pk_wide_gb_last_score(), pk_wide_gb_scene());
    fflush(stdout);
    capture_frame("runtime-phase8-before.png", NULL);
    assert(wideMode == 1 && wideReady);
    assert(pocket_display_load(root, displayKey) == 1);
    assert(lastGameRect.w == 1020 && lastGameRect.h == 760);
    assert(capture_frame("runtime-phase8-expand.png", NULL));
    send_button(13);
  }
  if (frame == 1850) {
    assert(wideMode == 2 && wideReady);
    assert(lastGameRect.w == 1024 && lastGameRect.h == 768);
    assert(capture_frame("runtime-phase8-zoom.png", NULL));
    send_key(SDLK_TAB);
    pausedCounter = c->frameCounter(c);
  }
  if (frame == 1851)
    assert(companion && c->frameCounter(c) == pausedCounter);
  if (frame == 1854) {
    assert(companion && c->frameCounter(c) == pausedCounter);
    send_key(SDLK_TAB);
  }
  if (frame == 1860) {
    assert(!companion && wideReady);
    send_key(SDLK_F3);
    pausedCounter = c->frameCounter(c);
  }
  if (frame == 1861) {
    assert(testQuick && c->frameCounter(c) == pausedCounter);
    send_key(SDLK_F3);
  }
  if (frame == 1865)
    send_key(SDLK_F4);
  if (frame == 1866) {
    assert(wideMode == 0 && !wideReady);
    assert(lastGameRect.w == 800 && lastGameRect.h == 720);
  }
  if (frame == 1870)
    send_key(SDLK_F4);
  if (frame == 1890) {
    assert(wideReady);
    puts("PASS R3 keyboard/raw, expand/zoom/original sizes, companion and "
         "wheel pause/resume");
  }
  if (frame == 1890 || frame == 1894) {
    SDL_Event e = {.type = frame == 1890 ? SDL_KEYDOWN : SDL_KEYUP};
    e.key.keysym.sym = SDLK_RETURN;
    SDL_PushEvent(&e);
  }
  if (frame == 1896) {
    assert(!wideReady && pk_wide_gb_wants_close());
    assert(capture_frame("runtime-phase8-menu-fallback.png", NULL));
    SDL_Event e = {.type = SDL_KEYDOWN};
    e.key.keysym.sym = SDLK_x;
    SDL_PushEvent(&e);
  }
  if (frame == 1902) {
    SDL_Event e = {.type = SDL_KEYUP};
    e.key.keysym.sym = SDLK_x;
    SDL_PushEvent(&e);
  }
  if (frame == 1920) {
    assert(wideReady);
    assert(state(c, "/tmp/pd-wide-roundtrip.ss", true));
    assert(state(c, "/tmp/pd-wide-roundtrip.ss", false));
    assert(!wideReady);
  }
  if (frame == 1940) {
    assert(wideReady);
    unlink("/tmp/pd-wide-roundtrip.ss");
    puts("PASS in-game menu fallback, state-load cache reset and exact PPU "
         "centre preservation");
  }
}
static void test_frame(struct mCore *c, int *pc, bool companion, int frame,
                       struct pd_ui *u, struct pd_snapshot *s,
                       const struct pc_ui *pcUi) {
  if (wideTest) {
    wide_test_frame(c, companion, frame);
    return;
  }
  if (quickTest) {
    quick_test_frame(c, companion, frame);
    return;
  }
  if (realRomTest) {
    if (frame == 0 && getenv("POCKET_TEST_STATE"))
      assert(state(c, getenv("POCKET_TEST_STATE"), false));
    if (frame == 119 && getenv("POCKET_TEST_STATE")) {
      assert(*pc >= 0 && s->partyCount > 0);
      printf("PASS actual emulator party: %d Lv%d HP%d/%d\n",
             s->party[0].species, s->party[0].level, s->party[0].hp,
             s->party[0].maxHp);
    }
    if (frame == 121 && getenv("POCKET_TEST_STATE"))
      assert(capture_frame("runtime-live323-companion.png", NULL));
    if (frame == 119) {
      unsigned lit = 0;
      assert(capture_frame("runtime-personal-rom-boot.png", &lit));
      assert(lit > 100);
      int ow, oh;
      SDL_GetRendererOutputSize(renderer, &ow, &oh);
      int expected = ow / 160;
      if (oh / 144 < expected)
        expected = oh / 144;
      assert(lastGameRect.w == 160 * expected &&
             lastGameRect.h == 144 * expected);
      printf("PASS physical-pixel game scaling %dx%d in %dx%d\n",
             lastGameRect.w, lastGameRect.h, ow, oh);
      pausedCounter = c->frameCounter(c);
      SDL_Event e = {.type = SDL_JOYBUTTONDOWN};
      e.jbutton.button = 3;
      SDL_PushEvent(&e);
    }
    if (frame == 120) {
      assert(companion);
      assert(c->frameCounter(c) == pausedCounter);
      SDL_Event e = {.type = SDL_JOYBUTTONUP};
      e.jbutton.button = 3;
      SDL_PushEvent(&e);
    }
    if (getenv("POCKET_TEST_STATE")) {
      if (frame >= 122 && frame < 122 + PC_MAP)
        send_key(SDLK_e);
      if (frame == 123) {
        assert(companion && pcUi->section == PC_BAG && s->bagOk);
        assert(c->frameCounter(c) == pausedCounter);
        assert(capture_frame("runtime-phase13-bag-sdl.png", NULL));
      }
      if (frame == 125) {
        assert(companion && pcUi->section == PC_DEX && s->dexOk);
        assert(c->frameCounter(c) == pausedCounter);
        assert(capture_frame("runtime-phase13-dex-sdl.png", NULL));
      }
      if (frame == 126) {
        assert(companion && pcUi->section == PC_MAP);
        assert(c->frameCounter(c) == pausedCounter);
        assert(capture_frame("runtime-phase12-map-sdl.png", NULL));
      }
      if (frame >= 126 && frame < 126 + PC_MAP)
        send_key(SDLK_q);
      if (frame == 131) {
        assert(companion && pcUi->section == PC_PARTY);
        send_key(SDLK_z);
      }
      if (frame == 133) {
        assert(companion && pcUi->summary);
        assert(c->frameCounter(c) == pausedCounter);
        assert(capture_frame("runtime-phase12-summary-sdl.png", NULL));
        send_key(SDLK_x);
      }
      if (frame == 134)
        send_key(SDLK_q);
      if (frame == 135) {
        assert(companion && pcUi->section == PC_GUIDE);
        assert(c->frameCounter(c) == pausedCounter);
        assert(capture_frame("runtime-phase14-guide-here-sdl.png", NULL));
        send_key(SDLK_LEFT);
      }
      if (frame == 136) {
        assert(pcUi->guidePagesFocused);
        send_key(SDLK_DOWN);
      }
      if (frame == 137) {
        assert(pcUi->guidePage == 2);
        send_key(SDLK_RIGHT);
      }
      if (frame == 138)
        send_key(SDLK_z);
      if (frame == 139) {
        assert(pcUi->guideReveal == 1);
        assert(capture_frame("runtime-phase14-guide-hint-sdl.png", NULL));
        send_key(SDLK_z);
      }
      if (frame == 140) {
        assert(pcUi->guideReveal == 2);
        assert(c->frameCounter(c) == pausedCounter);
        assert(capture_frame("runtime-phase14-guide-answer-sdl.png", NULL));
        send_key(SDLK_x);
      }
      if (frame == 141)
        assert(!pcUi->guideReveal && companion);
    }
    int exitAt = getenv("POCKET_TEST_STATE") ? 148 : 138;
    if (frame == exitAt) {
      assert(c->frameCounter(c) == pausedCounter);
      SDL_Event e = {.type = SDL_JOYBUTTONDOWN};
      e.jbutton.button = 0;
      SDL_PushEvent(&e);
    }
    if (frame == exitAt + 1) {
      assert(!companion);
      SDL_Event e = {.type = SDL_JOYBUTTONUP};
      e.jbutton.button = 0;
      SDL_PushEvent(&e);
    }
    if (frame == exitAt + 2) {
      assert(c->frameCounter(c) > pausedCounter);
      assert(state(c, "/tmp/pd-personal-rom-test-state", true));
      lastCounter = c->frameCounter(c);
    }
    if (frame == exitAt + 7) {
      assert(state(c, "/tmp/pd-personal-rom-test-state", false));
      assert(c->frameCounter(c) == lastCounter);
    }
    if (frame == 159)
      puts("PASS uploaded ROM: nonblack opaque RGB frame, joystick X/B "
           "pause/resume, real save-state round-trip; no fixture injection");
    return;
  }
  if (gbaTest) {
    if (frame == 0) {
      assert(c->platform(c) == mPLATFORM_GBA);
      const char *files[] = {"ewram.bin", "iwram.bin"};
      for (int i = 0; i < 2; i++) {
        char file[256];
        snprintf(file, sizeof file,
                 "../app/src/test/resources/fixtures/firered_vanilla/%s",
                 files[i]);
        FILE *f = fopen(file, "rb");
        assert(f);
        size_t size = 0;
        void *mem = c->getMemoryBlock(c, 2 + i, &size);
        assert(mem);
        assert(fread(mem, 1, size, f) == size);
        fclose(f);
      }
    }
    if (frame == 5) {
      assert(s->valid && s->partyCount > 0);
      send_key(SDLK_TAB);
    }
    if (frame == 6) {
      assert(companion);
      capture_frame("runtime-gba-party-fixture.png", NULL);
      send_key(SDLK_DOWN);
    }
    if (frame == 8)
      send_key(SDLK_z);
    if (frame == 10) {
      assert(u->summarySlot >= 0);
      capture_frame("runtime-gba-summary-fixture.png", NULL);
      send_key(SDLK_x);
    }
    if (frame == 12)
      send_key(SDLK_e);
    if (frame == 14) {
      assert(u->tab == PD_TAB_BAG);
      capture_frame("runtime-gba-bag-fixture.png", NULL);
      send_key(SDLK_TAB);
    }
    if (frame == 16)
      assert(!companion);
    if (frame == 18) {
      int ow, oh;
      SDL_GetRendererOutputSize(renderer, &ow, &oh);
      int scale = ow / 240;
      if (oh / 160 < scale)
        scale = oh / 160;
      assert(lastGameRect.w == 240 * scale && lastGameRect.h == 160 * scale);
      printf("PASS GBA physical-pixel scaling %dx%d in %dx%d\n", lastGameRect.w,
             lastGameRect.h, ow, oh);
      capture_frame("runtime-gba-diagnostic.png", NULL);
      puts("PASS real GBA execution and original companion party, summary, bag "
           "and return to game");
    }
    return;
  }
  (void)u;
  if (frame == 0) {
    assert(c->platform(c) == mPLATFORM_GB);
    struct GB *gb = c->board;
    FILE *f = fopen(
        "../app/src/test/resources/fixtures/polished_crystal/wramx.bin", "rb");
    assert(f);
    assert(fread(gb->memory.wram, 1, 32768, f) == 32768);
    fclose(f);
    *pc = 0; /* Fixture-only harness: never part of the release executable. */
  }
  if (frame == 5) {
    assert(c->frameCounter(c) > 0);
    assert(audioNonzero > 0);
    assert(c->busRead8(c, 0xa000) == 0x5a);
    assert(state(c, "/tmp/pd-state-test", true));
    lastCounter = c->frameCounter(c);
  }
  if (frame == 8) {
    assert(c->frameCounter(c) > lastCounter);
    c->busWrite8(c, 0xa000, 0x6b);
    assert(state(c, "/tmp/pd-state-test", false));
    assert(c->busRead8(c, 0xa000) == 0x6b);
    c->busWrite8(c, 0xa000, 0x5a);
    assert(c->frameCounter(c) == lastCounter);
  }
  if (frame == 9) {
    capture_frame("runtime-diagnostic.png", NULL);
    send_key(SDLK_TAB);
  }
  if (frame == 10) {
    assert(companion);
    pausedCounter = c->frameCounter(c);
    assert(s->partyCount == 2 && s->party[0].species == 155);
    capture_frame("runtime-party-fixture.png", NULL);
    send_key(SDLK_z);
  }
  if (frame == 12) {
    assert(companion);
    assert(c->frameCounter(c) == pausedCounter);
    capture_frame("runtime-summary-fixture.png", NULL);
    send_key(SDLK_x);
  }
  if (frame == 14)
    send_key(SDLK_TAB);
  if (frame == 16) {
    assert(!companion && c->frameCounter(c) > pausedCounter);
    send_key(SDLK_TAB);
  }
  if (frame == 17) {
    assert(companion);
    send_key(SDLK_e);
  }
  if (frame == 18) {
    assert(s->bagOk);
    assert(pcUi->section == PC_BAG && pcUi->bagStarted && pcUi->pocket == -1);
    capture_frame("runtime-polished-bag-fixture.png", NULL);
    send_key(SDLK_RIGHT);
  }
  if (frame == 19) {
    assert(pcUi->section == PC_BAG && pcUi->pocket == PD_POCKET_ITEMS);
    capture_frame("runtime-phase13-bag-pocket-sdl-fixture.png", NULL);
    send_key(SDLK_TAB);
  }
  if (frame >= 20 && frame < 120)
    send_key(SDLK_TAB);
  if (frame == 125) {
    assert(!companion);
    assert(c->busRead8(c, 0xa000) == 0x5a);
    puts("PASS real GB frames, battery marker, state round-trip, party fixture "
         "in live WRAM, SDL navigation and 100 companion toggles");
  }
  if (frame == 126 || frame == 146) {
    struct GB *gb = c->board;
    const char *fixture =
        frame == 126 ? "polished_crystal_battle" : "polished_crystal_trainer";
    char path[256];
    snprintf(path, sizeof path,
             "../app/src/test/resources/fixtures/%s/wramx.bin", fixture);
    FILE *f = fopen(path, "rb");
    assert(f);
    assert(fread(gb->memory.wram, 1, 32768, f) == 32768);
    fclose(f);
    if (frame == 126)
      send_key(SDLK_TAB);
  }
  if (frame == 127)
    send_key(SDLK_e);
  if (frame == 128) {
    assert(companion && s->inBattle && s->battlers[1].species == 161);
    assert(pcUi->section == PC_BATTLE);
    capture_frame("runtime-polished-battle-fixture.png", NULL);
    send_key(SDLK_e);
  }
  if (frame == 129) {
    assert(s->dexOk);
    assert(pcUi->section == PC_DEX);
  }
  if (frame >= 129 && frame <= 142)
    send_key(SDLK_RIGHT);
  if (frame == 143) {
    capture_frame("runtime-polished-dex-fixture.png", NULL);
    send_key(SDLK_z);
  }
  if (frame == 144) {
    capture_frame("runtime-polished-entry-fixture.png", NULL);
    send_key(SDLK_x);
  }
  if (frame == 145)
    send_key(SDLK_q);
  if (frame == 147) {
    assert(s->isTrainer && s->foeCount == 1 && s->foeActive == 0);
    capture_frame("runtime-polished-trainer-fixture.png", NULL);
    struct GB *gb = c->board;
    FILE *f = fopen(
        "../app/src/test/resources/fixtures/polished_crystal/wramx.bin", "rb");
    assert(f);
    assert(fread(gb->memory.wram, 1, 32768, f) == 32768);
    fclose(f);
    send_key(SDLK_q);
  }
  if (frame == 148)
    send_key(SDLK_z);
  if (frame == 149) {
    capture_frame("runtime-polished-item-fixture.png", NULL);
    send_key(SDLK_x);
  }
  if (frame == 150)
    send_key(SDLK_TAB);
  if (frame == 153) {
    assert(!companion);
    puts("PASS live wild/trainer battle, dex/list/detail, item info, section "
         "navigation and resume");
  }
}
int main(void) {
  if (getenv("POCKET_TEST_QUICK") || getenv("POCKET_TEST_WIDE")) {
    wideTest = getenv("POCKET_TEST_WIDE") != NULL;
    quickTest = true;
    unsigned char bits[(KEY_MAX + 7) / 8] = {0};
    int codes[] = {BTN_A, BTN_THUMBL, BTN_THUMBR, KEY_ENTER};
    for (int i = 0; i < 4; i++)
      bits[codes[i] / 8] |= 1u << (codes[i] % 8);
    int total = 0;
    assert(quick_evdev_button(bits, BTN_THUMBL, &total) == 1 && total == 4);
    assert(quick_evdev_button(bits, BTN_THUMBR, NULL) == 2);
    assert(quick_evdev_button(bits, KEY_ENTER, NULL) == 3);
    assert(mkdtemp(quickRoot));
    setenv("POCKET_L3_BUTTON", "12", 1);
    setenv("POCKET_R3_BUTTON", "13", 1);
    quickRom = getenv("POCKET_TEST_ROM");
    if (!quickRom)
      quickRom = "diagnostic.gb";
    char *args[] = {
        "pokedaisy-pocket",       "--data",         quickRoot, "--frames",
        wideTest ? "1942" : "68", (char *)quickRom, NULL};
    int result = pocket_app_main(6, args);
    if (wideTest) {
      wideReopen = true;
      args[4] = "3";
      result |= pocket_app_main(6, args);
    }
    return result;
  }
  const char *personal = getenv("POCKET_TEST_ROM");
  if (personal) {
    realRomTest = true;
    char *args[] = {"pokedaisy-pocket",
                    "--data",
                    "/tmp/pd-personal-rom-test",
                    "--frames",
                    "160",
                    (char *)personal,
                    NULL};
    return pocket_app_main(6, args);
  }

  char *args[] = {
      "pokedaisy-pocket", "--data", "/tmp/pd-runtime-test", "--frames", "160",
      "diagnostic.gb",    NULL};
  int result = pocket_app_main(6, args);
  FILE *save = fopen("/tmp/pd-runtime-test/saves/diagnostic.gb.sav", "rb");
  assert(save);
  assert(fgetc(save) == 0x5a);
  fclose(save);
  gbaTest = true;
  args[4] = "24";
  args[5] = "diagnostic.gba";
  result |= pocket_app_main(6, args);
  return result;
}
