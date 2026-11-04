/* PokeDaisy as a libretro core: mGBA's own libretro core (MPL-2.0, third_party/mgba, included below
 * unchanged) with the companion layered over it. GPL-3.0, see ../NOTICE.
 *
 * mGBA's entry points are renamed with the preprocessor while its file is included, so this file can
 * define the real retro_* functions around them and reach its static state (core, data, outputBuffer)
 * without editing a line of upstream. */
#define retro_set_environment mgba_retro_set_environment
#define retro_set_video_refresh mgba_retro_set_video_refresh
#define retro_set_input_state mgba_retro_set_input_state
#define retro_get_system_info mgba_retro_get_system_info
#define retro_get_system_av_info mgba_retro_get_system_av_info
#define retro_run mgba_retro_run
#define retro_load_game mgba_retro_load_game
#define retro_unload_game mgba_retro_unload_game
#define retro_unserialize mgba_retro_unserialize
#include "libretro.c"
#undef retro_set_environment
#undef retro_set_video_refresh
#undef retro_set_input_state
#undef retro_get_system_info
#undef retro_get_system_av_info
#undef retro_run
#undef retro_load_game
#undef retro_unload_game
#undef retro_unserialize

#include "field_items.h"
#include "fly.h"
#include "hud.h"
#include "pd_guide.h"
#include "pk_call.h"
#include "pk_wide.h"
#include "quick_menu.h"
#include "wild.h"
#include "pd_dex.h"
#include "pd_map.h"
#include "pd_menu.h"
#include "pd_romart.h"
#include "pd_settings.h"
#include "pd_ui.h"
#include "pd_ui_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

bool pd_lr_read_memory(void *ctx, uint32_t a, void *out, size_t n);

/* ---- keys: the retropad, as the SDL host's key enum numbers them (A..L are mGBA's own key bits) ---- */
enum { A, B, SELECT, START, RIGHT, LEFT, UP, DOWN, R, L, X, Y, L2, R2, MENU, L3, NKEYS };
static const unsigned retroId[NKEYS] = {
    RETRO_DEVICE_ID_JOYPAD_A,     RETRO_DEVICE_ID_JOYPAD_B,    RETRO_DEVICE_ID_JOYPAD_SELECT, RETRO_DEVICE_ID_JOYPAD_START,
    RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_UP,     RETRO_DEVICE_ID_JOYPAD_DOWN,
    RETRO_DEVICE_ID_JOYPAD_R,     RETRO_DEVICE_ID_JOYPAD_L,    RETRO_DEVICE_ID_JOYPAD_X,      RETRO_DEVICE_ID_JOYPAD_Y,
    RETRO_DEVICE_ID_JOYPAD_L2,    RETRO_DEVICE_ID_JOYPAD_R2,   RETRO_DEVICE_ID_JOYPAD_SELECT, RETRO_DEVICE_ID_JOYPAD_L3};

/* ---- the options this layer adds to mGBA's ---- */
#define PD_OPTION_COUNT 4
static const struct retro_core_option_v2_definition pdOptions[PD_OPTION_COUNT] = {
    {"pokedaisy_canvas", "Companion canvas", NULL, "The frame size the core reports: the game sits in it at a whole scale, the companion fills it. auto = 320x240.", NULL, NULL,
     {{"auto", NULL}, {"320x240", NULL}, {"341x256", NULL}, {"400x300", NULL}, {"427x240", NULL}, {NULL, NULL}}, "auto"},
    {"pokedaisy_quick_button", "Quick menu button", NULL, "Opens the quick menu over the game.", NULL, NULL,
     {{"Y", NULL}, {"X", NULL}, {"R2", NULL}, {"L2", NULL}, {"L3", NULL}, {NULL, NULL}}, "Y"},
    {"pokedaisy_companion_button", "Companion button", NULL, "Opens the companion screens.", NULL, NULL,
     {{"X", NULL}, {"Y", NULL}, {"R2", NULL}, {"L2", NULL}, {"L3", NULL}, {NULL, NULL}}, "X"},
    {"pokedaisy_hud_button", "HUD cursor button", NULL, "Puts the cursor on the HUD over the overworld.", NULL, NULL,
     {{"R2", NULL}, {"L2", NULL}, {"L3", NULL}, {"X", NULL}, {"Y", NULL}, {NULL, NULL}}, "R2"},
};

static retro_environment_t realEnv;
static retro_video_refresh_t realVideo;
static retro_input_state_t realInput;
static bool optionsDirty;

static const char *option(const char *key) {
  struct retro_variable v = {key, NULL};
  return realEnv && realEnv(RETRO_ENVIRONMENT_GET_VARIABLE, &v) && v.value ? v.value : NULL;
}

/* mGBA defines its options in one call; ours go in with them (an environment may take only one). */
static bool env_wrap(unsigned cmd, void *data) {
  if ((cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL || cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2) && data) {
    const struct retro_core_options_v2 *us =
        cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL ? ((struct retro_core_options_v2_intl *)data)->us : data;
    size_t n = 0;
    while (us && us->definitions[n].key)
      n++;
    struct retro_core_option_v2_definition *defs = calloc(n + PD_OPTION_COUNT + 1, sizeof *defs);
    if (defs && us) {
      memcpy(defs, us->definitions, n * sizeof *defs);
      memcpy(defs + n, pdOptions, sizeof pdOptions);
      static struct retro_core_options_v2 merged;
      static struct retro_core_options_v2_intl mergedIntl;
      merged = (struct retro_core_options_v2){us->categories, defs};
      mergedIntl = (struct retro_core_options_v2_intl){&merged, NULL};
      return realEnv(cmd, cmd == RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL ? (void *)&mergedIntl : (void *)&merged);
    }
  }
  bool r = realEnv(cmd, data);
  if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE && data && *(bool *)data)
    optionsDirty = true;
  return r;
}

/* ---- the frame: the canvas the frontend sees, and where the game and the overlays are drawn ---- */
#define CANVAS_MAX_W 640
#define CANVAS_MAX_H 480
static int CW = 320, CH = 240;
static uint32_t *cvs;     // the companion's canvas, 0xRRGGBB
static uint16_t *out565;  // what the frontend gets
static struct pd_canvas canvas;
static unsigned gameW, gameH;
static bool gameShown;

static char message[128];
static int messageFrames;
static void notice(const char *s) {
  snprintf(message, sizeof message, "%s", s);
  messageFrames = 90;
  fprintf(stderr, "pokedaisy: %s\n", s);
}

static inline uint32_t from565(uint16_t p) {
  uint32_t r = (p >> 11) & 31, g = (p >> 5) & 63, b = p & 31;
  return ((r << 3 | r >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (b << 3 | b >> 2);
}
static inline uint16_t to565(uint32_t p) {
  return (uint16_t)(((p >> 8) & 0xf800) | ((p >> 5) & 0x07e0) | ((p >> 3) & 0x001f));
}

/* EXPAND (the game-code layer, below): the window it rebuilt past the game's edges, when this frame has one. */
static bool wideSupported, wideReady, fieldKnown;
static unsigned wideW, wideH;
static uint32_t wideSource[240 * 160], wideFrame[PK_WIDE_MAX_VIEW_W * PK_WIDE_MAX_VIEW_H];
static uint32_t wildSwitch;
static inline uint32_t to_xbgr(uint16_t p) {
  uint32_t r = (p >> 11) & 31, g = (p >> 5) & 63, b = p & 31;
  return 0xFF000000u | ((b << 3 | b >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (r << 3 | r >> 2);
}
static inline uint32_t xbgr_to_rgb(uint32_t p) { return (p & 255) << 16 | (p >> 8 & 255) << 8 | (p >> 16 & 255); }

/* The game, whole-scale and centred, over a black canvas. */
static void draw_game(void) {
  memset(cvs, 0, (size_t)CW * CH * sizeof *cvs);
  if (!outputBuffer || !gameW || !gameH)
    return;
  if (wideReady) {
    int ws = CW / (int)wideW < CH / (int)wideH ? CW / (int)wideW : CH / (int)wideH;
    if (ws < 1)
      ws = 1;
    int wx = (CW - (int)wideW * ws) / 2, wy = (CH - (int)wideH * ws) / 2;
    for (unsigned y = 0; y < wideH; y++)
      for (int k = 0; k < ws; k++) {
        int cy = wy + (int)y * ws + k;
        if (cy < 0 || cy >= CH)
          continue;
        for (unsigned x = 0; x < wideW; x++) {
          uint32_t p = xbgr_to_rgb(wideFrame[y * wideW + x]);
          for (int j = 0; j < ws; j++) {
            int cx = wx + (int)x * ws + j;
            if (cx >= 0 && cx < CW)
              cvs[(size_t)cy * CW + cx] = p;
          }
        }
      }
    return;
  }
  int s = CW / (int)gameW < CH / (int)gameH ? CW / (int)gameW : CH / (int)gameH;
  if (s < 1)
    s = 1;
  int ox = (CW - (int)gameW * s) / 2, oy = (CH - (int)gameH * s) / 2;
  for (unsigned y = 0; y < gameH; y++) {
    uint32_t *row = cvs + (size_t)(oy + (int)y * s) * CW + ox;
    if (oy + (int)y * s < 0 || oy + ((int)y + 1) * s > CH)
      continue;
    for (unsigned x = 0; x < gameW; x++) {
      uint32_t p = from565(outputBuffer[y * 256 + x]);
      for (int k = 0; k < s; k++)
        row[x * s + k] = p;
    }
    for (int k = 1; k < s; k++)
      memcpy(row + (size_t)k * CW, row, (size_t)gameW * s * sizeof *row);
  }
}

/* An overlay drawn over QUICK_CLEAR and finished (quick_finish): its alpha laid over the canvas at the
 * given integer scale, its top-left at (x, y) in canvas pixels. */
static void blend_overlay(const uint32_t *px, int w, int h, int scale, int x0, int y0) {
  for (int y = 0; y < h * scale; y++) {
    int cy = y0 + y;
    if (cy < 0 || cy >= CH)
      continue;
    for (int x = 0; x < w * scale; x++) {
      int cx = x0 + x;
      if (cx < 0 || cx >= CW)
        continue;
      uint32_t p = px[(y / scale) * w + x / scale], a = p >> 24;
      if (!a)
        continue;
      uint32_t *d = &cvs[(size_t)cy * CW + cx];
      if (a == 255) {
        *d = p & 0xffffff;
      } else {
        uint32_t dr = *d >> 16 & 255, dg = *d >> 8 & 255, db = *d & 255;
        uint32_t r = (dr * (255 - a) + (p >> 16 & 255) * a) / 255, g = (dg * (255 - a) + (p >> 8 & 255) * a) / 255,
                 b = (db * (255 - a) + (p & 255) * a) / 255;
        *d = r << 16 | g << 8 | b;
      }
    }
  }
}

static void dim(int alpha) {
  for (size_t i = 0; i < (size_t)CW * CH; i++) {
    uint32_t p = cvs[i];
    cvs[i] = (((p >> 16 & 255) * (255 - alpha) / 255) << 16) | (((p >> 8 & 255) * (255 - alpha) / 255) << 8) |
             ((p & 255) * (255 - alpha) / 255);
  }
}

/* Convert and hand the finished canvas to the frontend. */
static void present(void) {
  if (messageFrames > 0) {
    pd_fill(&canvas, 4, CH - 50, CW - 8, 24, 0x181818);
    pd_text_fit(&canvas, 10, CH - 46, CW - 20, message, 0xffffff, PD_NO_SHADOW);
    messageFrames--;
  }
  for (size_t i = 0; i < (size_t)CW * CH; i++)
    out565[i] = to565(cvs[i]);
  realVideo(out565, (unsigned)CW, (unsigned)CH, (size_t)CW * 2);
}

/* ---- input ---- */
static unsigned held, prevHeld, pressedKeys;
static int heldFrames[NKEYS];
static int quickBtn = Y, companionBtn = X, hudBtn = R2;
static unsigned gameKeys; // keys the layer presses for the game (BIKE's SELECT)
static int gameKeyFrames;
static bool maskGame;     // while an overlay is up the game sees no keys

static int button_named(const char *s, int def) {
  static const struct { const char *n; int k; } names[] = {{"X", X}, {"Y", Y}, {"R2", R2}, {"L2", L2}, {"L3", L3}};
  for (unsigned i = 0; s && i < sizeof names / sizeof *names; i++)
    if (!strcmp(s, names[i].n))
      return names[i].k;
  return def;
}
static void read_options(void) {
  quickBtn = button_named(option("pokedaisy_quick_button"), Y);
  companionBtn = button_named(option("pokedaisy_companion_button"), X);
  hudBtn = button_named(option("pokedaisy_hud_button"), R2);
}

static void poll_keys(void) {
  held = 0;
  if (realInput)
    for (int k = 0; k < NKEYS; k++)
      if (k != MENU && realInput(0, RETRO_DEVICE_JOYPAD, 0, retroId[k]))
        held |= 1u << k;
  pressedKeys = held & ~prevHeld;
  for (int k = 0; k < NKEYS; k++) {
    heldFrames[k] = (held >> k & 1) ? heldFrames[k] + 1 : 0;
    // The D-pad repeats while held: after a quarter second, every ~90 ms.
    if (k >= RIGHT && k <= DOWN && heldFrames[k] > 15 && (heldFrames[k] - 15) % 5 == 0)
      pressedKeys |= 1u << k;
  }
  prevHeld = held;
}
static bool down(int k) { return (pressedKeys >> k & 1) != 0; }

/* What mGBA reads while its frame runs. */
static int16_t masked_input(unsigned port, unsigned device, unsigned index, unsigned id) {
  if (port != 0 || device != RETRO_DEVICE_JOYPAD)
    return 0;
  if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
    int16_t m = 0;
    for (int k = A; k <= L; k++)
      if (maskGame ? (gameKeys >> k & 1) : ((held >> k & 1) || (gameKeyFrames > 0 && (gameKeys >> k & 1))))
        m |= 1 << retroId[k];
    return m;
  }
  for (int k = A; k <= L; k++)
    if (retroId[k] == id)
      return maskGame ? (gameKeyFrames > 0 && (gameKeys >> k & 1)) : ((held >> k & 1) || (gameKeyFrames > 0 && (gameKeys >> k & 1)));
  return maskGame ? 0 : realInput(port, device, index, id);
}

/* ---- the companion ---- */
#include "pd_focus.inc"

static struct pd_game game;
static struct pd_settings settings;
static struct pd_ui ui;
static struct pd_snapshot snap;
static struct pd_host_info host;
static char settingsPath[1100];
static bool companion, companionWas, haveGame;
static int frames;

/* the HUD over the overworld (B2) */
static struct hud_state hudState = {0, 0, false, -1, -1};
static struct hud_hits hudHits;
static bool hudFocus, hudShown;
static uint32_t *hudPixels;
static uint32_t hudDrawnKey;
static int hudW, hudH;
static int fieldFlags, hudFieldFrames;

/* the quick menu and the game-code jobs: PC, FLY, REPEL, BIKE (fly.c, field_items.c) */
static bool quickOpen;
static int quickSel;
static const struct pd_config *flyCfg;
static struct fly_targets flyT;
static const char *flyNames[FLY_MAX_TOWNS];
static int flyAsk, flySel, flyStage, flyMapsec, flyWait;
static bool flyList;
static uint32_t flyCallback;
static bool itemsOk, pcOk, bikeOk;
static enum { JOB_NONE, JOB_PC, JOB_REPEL, JOB_BIKE, JOB_BIKE_WAIT, JOB_BIKE_RESTORE } job;
static int jobItem, jobFrames, jobTries, repelSel, repelKinds[REPEL_KINDS], repelKindCount;
static bool repelList, bikeWasRiding, bikeTook;
static uint16_t bikeOld;
static char repelRows[REPEL_KINDS][48];
static const char *repelNames[REPEL_KINDS];
static uint32_t *ovPixels; // the quick menu's canvas, see-through where nothing is drawn
static const unsigned char *romData;
static size_t romLen;


static void snapshot(void) { pd_snapshot_read(&snap, &game, pd_lr_read_memory, core); }

static int hud_wild_uncaught(const struct pd_game *g, const struct pd_snapshot *s) {
  struct pd_wild_set w;
  if (!g->cfg || !g->cfg->wildHeaders)
    return -1;
  if (!pd_wild_here(g, s, &w))
    return 0;
  int seen[PD_WILD_METHODS * PD_WILD_MAX], n = 0, left = 0;
  for (int m = 0; m < PD_WILD_METHODS; m++)
    for (int i = 0; i < w.count[m]; i++) {
      int sp = w.mons[m][i].species, k = 0;
      while (k < n && seen[k] != sp)
        k++;
      if (k < n)
        continue;
      seen[n++] = sp;
      left += !pd_dex_caught(s, pd_dex_national(g, sp));
    }
  return left;
}

/* The HUD over the game's own picture, at the companion's pixel scale (SIZE steps it). */
static void draw_hud(void) {
  int gs = CW / (int)gameW < CH / (int)gameH ? CW / (int)gameW : CH / (int)gameH;
  if (gs < 1)
    gs = 1;
  int ax = (CW - (int)gameW * gs) / 2, ay = (CH - (int)gameH * gs) / 2, aw = (int)gameW * gs, ah = (int)gameH * gs;
  int scale = 1 + settings.hudSize - PD_HUD_NORMAL;
  if (scale < 1)
    scale = 1;
  int w = aw / scale, hh = ah / scale;
  if (!hudPixels)
    hudPixels = malloc((size_t)CANVAS_MAX_W * CANVAS_MAX_H * sizeof *hudPixels);
  if (!hudPixels)
    return;
  uint32_t key = hud_key(&snap, &hudState, &settings) ^ (uint32_t)(w * 65599 + hh);
  if (key != hudDrawnKey || w != hudW || hh != hudH) {
    struct pd_canvas hc;
    pd_canvas_init(&hc, w, hh, hudPixels);
    hudShown = hud_draw(&hc, &game, &snap, &hudState, &settings, &hudHits);
    quick_finish(&hc);
    hudDrawnKey = key;
    hudW = w;
    hudH = hh;
  }
  if (hudShown)
    blend_overlay(hudPixels, w, hh, scale, ax + (aw - w * scale) / 2, ay + (ah - hh * scale) / 2);
}


/* ---- game-code layer: EXPAND (pk_wide), the field read, WILD ENCOUNTERS ---- */


/* The EXPAND window for the canvas and a fw x fh game: the canvas over the game's whole scale (one less for
 * ZOOM OUT), within pk_wide's largest view, never smaller than the game, even margins. */
static void wide_window(int aspect, unsigned fw, unsigned fh, unsigned *w, unsigned *h) {
  int scale = CW / (int)fw < CH / (int)fh ? CW / (int)fw : CH / (int)fh;
  if (aspect == PD_ASPECT_ZOOM_OUT)
    scale--;
  if (scale < 1)
    scale = 1;
  unsigned x = (unsigned)CW / scale, y = (unsigned)CH / scale;
  x = x > PK_WIDE_MAX_VIEW_W ? PK_WIDE_MAX_VIEW_W : x < fw ? fw : x;
  y = y > PK_WIDE_MAX_VIEW_H ? PK_WIDE_MAX_VIEW_H : y < fh ? fh : y;
  *w = x - ((x - fw) & 1);
  *h = y - ((y - fh) & 1);
}

/* After every game frame: pk_wide's RAM history, and (wideReady) this frame's window. */
static void wide_tick(void) {
  wideReady = false;
  if (!wideSupported)
    return;
  unsigned fw = (unsigned)pk_wide_frame_w(), fh = (unsigned)pk_wide_frame_h();
  for (unsigned y = 0; y < fh; y++)
    for (unsigned x = 0; x < fw; x++)
      wideSource[y * fw + x] = to_xbgr(outputBuffer[y * 256 + x]);
  if (settings.aspect == PD_ASPECT_ORIGINAL) {
    // Nothing past the picture, but the scene still read (FLY's "on the field"): a one-pixel window.
    static uint32_t px;
    pk_wide_view(core, wideSource, &px, (struct PkWideView){0, 0, 1, 1}, NULL);
    return;
  }
  int last = pk_wide_scene();
  int aspect = settings.aspect == PD_ASPECT_ZOOM_OUT && settings.autoZoom && last != PK_WIDE_SCENE_FIELD
                   ? PD_ASPECT_EXPAND : settings.aspect;
  wide_window(aspect, fw, fh, &wideW, &wideH);
  struct PkWideView v = {((int)fw - (int)wideW) / 2, ((int)fh - (int)wideH) / 2, (int)wideW, (int)wideH};
  pk_wide_view(core, wideSource, wideFrame, v, NULL);
  wideReady = !pk_wide_fault();
}

static void choose_canvas(void) {
  const char *v = option("pokedaisy_canvas");
  int w = 320, h = 240;
  if (v && sscanf(v, "%dx%d", &w, &h) != 2)
    w = 320, h = 240;
  CW = w < 320 ? 320 : w > CANVAS_MAX_W ? CANVAS_MAX_W : w;
  CH = h < 240 ? 240 : h > CANVAS_MAX_H ? CANVAS_MAX_H : h;
}

/* ---- entry points ---- */
RETRO_API void retro_set_environment(retro_environment_t e) {
  realEnv = e;
  mgba_retro_set_environment(env_wrap);
}
/* What the capture of mGBA's own video call records: the size; the pixels stay in outputBuffer. */
static void capture_video(const void *d, unsigned w, unsigned h, size_t pitch) {
  (void)d;
  (void)pitch;
  gameW = w;
  gameH = h;
}
RETRO_API void retro_set_video_refresh(retro_video_refresh_t v) {
  realVideo = v;
  mgba_retro_set_video_refresh(capture_video);
}
RETRO_API void retro_set_input_state(retro_input_state_t i) {
  realInput = i;
  mgba_retro_set_input_state(masked_input);
}
RETRO_API void retro_get_system_info(struct retro_system_info *info) {
  mgba_retro_get_system_info(info);
  info->library_name = "PokeDaisy";
  info->library_version = "0.3";
}
RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info) {
  mgba_retro_get_system_av_info(info);
  info->geometry.base_width = info->geometry.max_width = (unsigned)CW;
  info->geometry.base_height = info->geometry.max_height = (unsigned)CH;
  info->geometry.aspect_ratio = (float)CW / (float)CH;
}

RETRO_API bool retro_load_game(const struct retro_game_info *g) {
  if (!mgba_retro_load_game(g))
    return false;
  choose_canvas();
  read_options();
  cvs = calloc((size_t)CW * CH, sizeof *cvs);
  out565 = calloc((size_t)CW * CH, sizeof *out565);
  if (!cvs || !out565)
    return false;
  pd_canvas_init(&canvas, CW, CH, cvs);
  uint32_t crc = 0;
  core->checksum(core, &crc, mCHECKSUM_CRC32);
  const unsigned char *rom = data;
  size_t len = dataSize;
  pd_game_detect(&game, rom, len);
  game.rom = rom;
  game.romSize = len;
  pd_game_identify(&game);
  fprintf(stderr, "pokedaisy: %s, sha1 %s%s%s\n", game.title ? game.title : "?", game.sha1[0] ? game.sha1 : "-",
          game.unsupported ? " - " : "", game.unsupported ? game.unsupported : "");
  if (core->platform(core) == mPLATFORM_GBA)
    pd_romart_scan(&game);
  const char *dir = NULL;
  if (realEnv(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir) {
    char sub[1050];
    snprintf(sub, sizeof sub, "%s/pokedaisy", dir);
    mkdir(sub, 0755);
    snprintf(settingsPath, sizeof settingsPath, "%s/settings.ini", sub);
  } else
    snprintf(settingsPath, sizeof settingsPath, "pokedaisy-settings.ini");
  pd_settings_load(&settings, settingsPath);
  wideSupported = pk_wide_configure(crc) != 0;
  wideReady = false;
  fieldKnown = core->platform(core) == mPLATFORM_GBA && pk_field_configure(crc);
  wildSwitch = fieldKnown ? pk_wild_switch_find(rom, len) : 0;
  if (wildSwitch)
    pk_field_wild(wildSwitch, settings.wildOff);
  pk_rest_reset();
  fieldFlags = 0;
  romData = rom;
  romLen = len;
  bool gba = core->platform(core) == mPLATFORM_GBA;
  flyCfg = game.cfg && gba && fly_supported(game.cfg, rom, len) ? game.cfg : NULL;
  itemsOk = game.cfg && gba && field_items_supported(game.cfg, rom, len);
  pcOk = itemsOk && pc_supported(game.cfg, rom, len);
  bikeOk = itemsOk && bike_supported(game.cfg);
  quickOpen = flyList = repelList = false;
  flyAsk = flyStage = 0;
  job = JOB_NONE;
  quick_set_grid(settings.quickGrid);
  ovPixels = calloc((size_t)CW * CH, sizeof *ovPixels);
  pd_ui_init(&ui, &settings);
  memset(&host, 0, sizeof host);
  host.hideScreenMode = true;
  host.hasQuickMenu = true;
  host.hasHud = core->platform(core) == mPLATFORM_GBA;
  host.hasExpand = true;
  host.expandGame = wideSupported;
  host.count = 1;
  host.labels[0] = "HOST";
  snprintf(host.values[0], 40, "PokeDaisy core");
  focusId = -1;
  focus_new_screen();
  companion = hudFocus = false;
  haveGame = true;
  return true;
}

RETRO_API bool retro_unserialize(const void *d, size_t n) {
  bool r = mgba_retro_unserialize(d, n);
  if (r && haveGame) {
    pk_wide_reset(); // EXPAND's RAM history is from before the jump
    wideReady = false;
    snap.valid = false;
  }
  return r;
}

RETRO_API void retro_unload_game(void) {
  pk_wide_configure(0);
  wideSupported = wideReady = false;
  mgba_retro_unload_game();
  free(cvs);
  free(out565);
  free(hudPixels);
  free(ovPixels);
  ovPixels = NULL;
  cvs = NULL;
  out565 = NULL;
  hudPixels = NULL;
  haveGame = false;
}

RETRO_API void retro_run(void) {
  if (!haveGame) {
    mgba_retro_run();
    return;
  }
  if (optionsDirty) {
    optionsDirty = false;
    read_options();
  }
  poll_keys();
  bool wasCompanion = companion;
  if (down(companionBtn) && !hudFocus) {
    companion = !companion;
    quickOpen = flyList = repelList = false;
    flyAsk = 0;
  }
  // The HUD button: the cursor goes onto the HUD, the D-pad moves it, A presses what it is on, B / the button leave.
  if (hudFocus && (companion || !hudShown)) {
    hudFocus = false;
    hudState.focus = -1;
  }
  if (down(hudBtn) && !companion) {
    if (hudFocus || !hudShown || !hudHits.n) {
      if (!hudFocus)
        notice("No HUD on screen");
      hudFocus = false;
      hudState.focus = -1;
    } else {
      hudFocus = true;
      hudState.focus = hudHits.at[0].id;
      for (int i = 0, lowest = PD_PARTY_SIZE; i < hudHits.n; i++)
        if (hudHits.at[i].id >= HUD_SLOT && hudHits.at[i].id - HUD_SLOT < lowest) {
          lowest = hudHits.at[i].id - HUD_SLOT;
          hudState.focus = hudHits.at[i].id;
        }
    }
  } else if (hudFocus) {
    if (down(B)) {
      hudFocus = false;
      hudState.focus = -1;
    } else if (down(A)) {
      int id = hudState.focus;
      hudFocus = false;
      hudState.focus = -1;
      if (id >= HUD_SLOT && id < HUD_SLOT + PD_PARTY_SIZE) {
        ui.tab = PD_TAB_PARTY;
        ui.summarySlot = id - HUD_SLOT;
      } else if (id == HUD_MAP || id == HUD_MAP_BUTTON) {
        ui.tab = PD_TAB_MAP;
        ui.mapPage = -1;
      } else if (id == HUD_BALLS || id == HUD_REPEL || id == HUD_BAG) {
        ui.tab = PD_TAB_BAG;
      } else if (id == HUD_WILD) {
        ui.tab = PD_TAB_GUIDE;
        ui.guidePage = 0;
        ui.guideOpen = -1;
      }
      if (id != HUD_PC)
        companion = true;
    } else {
      int dir = down(UP) ? UP : down(DOWN) ? DOWN : down(LEFT) ? LEFT : down(RIGHT) ? RIGHT : -1;
      if (dir >= 0)
        hudState.focus = hud_step(&hudHits, hudState.focus, dir);
    }
  }


  // The QUICK MENU over the paused game, in the app's order (QuickAction): PC, FLY, REPEL, BIKE, WILD, COMPANION;
  // each only where the game can, dimmed with why when not now.
  struct quick_item quick[QUICK_COUNT];
  int quickN = 0;
  char repelWhy[32] = "";
  if (pcOk)
    quick[quickN++] = (struct quick_item){QUICK_PC, false, false, NULL, 0, 0, NULL};
  if (flyCfg) {
    int flyer = fly_flyer(&snap);
    quick[quickN++] = (struct quick_item){QUICK_FLY, false, false, flyer < 0 ? "No one in your party knows FLY" : NULL,
                                          flyer < 0 ? 0 : snap.party[flyer].species, 0, NULL};
  }
  if (itemsOk) {
    repelKindCount = 0;
    for (int k = 0; k < REPEL_KINDS; k++)
      if (bag_count(&snap, repel_kinds[k][0]) > 0)
        repelKinds[repelKindCount++] = repel_kinds[k][0];
    int left = repel_steps(core, game.cfg);
    if (left)
      snprintf(repelWhy, sizeof repelWhy, "%d steps left", left);
    quick[quickN++] = (struct quick_item){QUICK_REPEL, false, false,
                                          left ? repelWhy : repelKindCount ? NULL : "No REPEL in your bag", 0,
                                          repelKindCount ? repelKinds[repelKindCount - 1] : repel_kinds[0][0], NULL};
  }
  if (bikeOk) {
    int bike = bag_bike(&snap);
    bool riding = (fieldFlags & PK_FIELD_AVATAR) && (fieldFlags & PK_FIELD_BIKE);
    quick[quickN++] = (struct quick_item){QUICK_BIKE, false, false, bike ? NULL : "No bike in your bag", 0, bike ? bike : 360,
                                          riding ? "GET OFF" : NULL};
  }
  quick[quickN++] = (struct quick_item){QUICK_WILD, settings.wildOff, false, wildSwitch ? NULL : "Not in this game", 0, 0, NULL};
  quick[quickN++] = (struct quick_item){QUICK_COMPANION, false, false, NULL, 0, 0, NULL};
  if (quickSel >= quickN)
    quickSel = 0;
  quick_set_grid(settings.quickGrid);
  if (down(quickBtn) && !companion && !hudFocus) {
    quickOpen = !quickOpen;
    flyList = repelList = false;
    flyAsk = 0;
  } else if (quickOpen && repelList) {
    if (down(UP))
      repelSel = (repelSel + repelKindCount - 1) % repelKindCount;
    if (down(DOWN))
      repelSel = (repelSel + 1) % repelKindCount;
    if (down(B))
      repelList = false;
    else if (down(A)) {
      quickOpen = repelList = false;
      job = JOB_REPEL;
      jobItem = repelKinds[repelSel];
      jobFrames = 0;
    }
  } else if (quickOpen && flyList) {
    if (down(UP) && flyT.count)
      flySel = (flySel + flyT.count - 1) % flyT.count;
    if (down(DOWN) && flyT.count)
      flySel = (flySel + 1) % flyT.count;
    if (down(B))
      flyList = false;
    else if (down(A) && flyT.count) {
      quickOpen = flyList = false;
      flyMapsec = flyT.towns[flySel];
      flyStage = 1;
      flyWait = 0;
    }
  } else if (quickOpen && down(B)) {
    quickOpen = false;
  } else if (quickOpen) {
    int dx = down(RIGHT) - down(LEFT), dy = down(DOWN) - down(UP);
    if (dx || dy)
      quickSel = quick_step(quickN, quickSel, dx, dy, CW, CH);
    if (down(A) && !quick[quickSel].why && quick[quickSel].action == QUICK_FLY) {
      flyAsk = 1; // answered below, between two frames
    } else if (down(A) && !quick[quickSel].why && quick[quickSel].action == QUICK_REPEL && repelKindCount > 1) {
      for (int k = 0; k < repelKindCount; k++) {
        int steps = 0;
        for (int r = 0; r < REPEL_KINDS; r++)
          if (repel_kinds[r][0] == repelKinds[k])
            steps = repel_kinds[r][1];
        snprintf(repelRows[k], sizeof repelRows[k], "%s x%d  %d steps", pd_item_name(&game, repelKinds[k]),
                 bag_count(&snap, repelKinds[k]), steps);
        repelNames[k] = repelRows[k];
      }
      repelList = true;
      repelSel = repelKindCount - 1;
    } else if (down(A) && !quick[quickSel].why) {
      quickOpen = false;
      switch (quick[quickSel].action) {
      case QUICK_PC:
        job = JOB_PC;
        jobFrames = 0;
        break;
      case QUICK_REPEL:
        job = JOB_REPEL;
        jobItem = repelKinds[0];
        jobFrames = 0;
        break;
      case QUICK_BIKE:
        job = JOB_BIKE;
        jobItem = bag_bike(&snap);
        jobFrames = 0;
        jobTries = 15; // PokeDaisyActivity's SELECT_TRIES
        break;
      case QUICK_WILD:
        settings.wildOff = !settings.wildOff;
        pd_settings_save(&settings, settingsPath);
        notice(settings.wildOff ? "Wild encounters off" : "Wild encounters on");
        break;
      case QUICK_COMPANION:
        companion = true;
        break;
      default:
        break;
      }
    }
  }

  if (companion) {
    if (down(B) && !map_cursor_back(&ui) && !pd_ui_back(&ui))
      companion = false;
    if (down(L) || down(R))
      pd_ui_next_tab(&ui, down(R) ? 1 : -1);
    for (int k = RIGHT; k <= DOWN; k++)
      if (down(k))
        focus_move(&ui, k);
    if (down(A))
      focus_activate(&ui);
    switch (pd_ui_take_action(&ui)) {
    case PD_ACTION_SETTINGS_CHANGED:
      pd_settings_save(&settings, settingsPath);
      hudDrawnKey = 0;
      break;
    default:
      break;
    }
  }

  bool paused = companion || hudFocus || (quickOpen && !flyAsk);
  maskGame = paused;
  if (!paused) {
    mgba_retro_run();
    if (gameKeyFrames > 0)
      gameKeyFrames--;
    if (core->platform(core) == mPLATFORM_GBA)
      pk_rest_note(core);
    if (fieldKnown)
      fieldFlags = pk_field_state(core, settings.runIndoors, wildSwitch && settings.wildOff ? 0 : 100);
    wide_tick();
  } else {
    static const int16_t silence[2 * 1024];
    audioCallback(silence, 549);
  }

  if (!snap.valid || (companion && !wasCompanion) || (!companion && frames % 30 == 0))
    snapshot();
  companionWas = companion;
  frames++;

  // FLY's steps and the field-item jobs, each between two frames the game's own code can be called in.
  bool flyBattle = snap.inBattle || (fieldFlags & PK_FIELD_BATTLE);
  bool flyBusy = companion || (fieldFlags & PK_FIELD_LOCKED) || (wideSupported && pk_wide_scene() != PK_WIDE_SCENE_FIELD);
  if (flyCfg && flyAsk && quickOpen && !companion) {
    if (pk_call_safe(core)) {
      snapshot();
      fly_targets(core, flyCfg, romData, romLen, &snap, flyBusy, flyBattle, &flyT);
      flyAsk = 0;
      if (flyT.result == FLY_OK && flyT.count) {
        for (int i = 0; i < flyT.count; i++) {
          const char *name = pd_mapsec_name(&game, flyT.towns[i]);
          flyNames[i] = name && *name ? name : "?";
        }
        flyList = true;
        flySel = 0;
      } else {
        quickOpen = false;
        notice(flyT.result == FLY_OK ? "You haven't been anywhere to FLY to yet." : fly_reason(flyT.result));
      }
    } else if (++flyAsk > 120) {
      flyAsk = 0;
      quickOpen = false;
      notice(fly_reason(FLY_FAILED));
    }
  }
  if (flyCfg && flyStage == 1 && !companion && pk_call_safe(core)) {
    snapshot();
    enum fly_result r = fly_start(core, flyCfg, romData, romLen, &snap, flyMapsec, &flyT, flyBusy, flyBattle);
    if (r == FLY_OK) {
      flyStage = 2;
      flyWait = 0;
      flyCallback = fly_main_callback(core, flyCfg);
    } else {
      flyStage = 0;
      notice(fly_reason(r));
    }
  } else if (flyCfg && flyStage == 2 && !companion && ++flyWait >= FLY_FADE_FRAMES && pk_call_safe(core)) {
    flyStage = 0;
    if (!fly_finish(core, flyCfg, romData, romLen, flyMapsec, &flyT, fly_main_callback(core, flyCfg) != flyCallback))
      notice(fly_reason(FLY_FAILED));
  }
  bool safe = job != JOB_NONE && !companion && pk_call_safe(core);
  if (job == JOB_PC && safe) {
    job = JOB_NONE;
    enum item_result r = pc_open(core, game.cfg, romData, romLen, flyBusy, flyBattle);
    if (r != ITEM_USED)
      notice(item_reason(r));
  } else if (job == JOB_REPEL && safe) {
    job = JOB_NONE;
    int steps = 0;
    enum item_result r = repel_use(core, game.cfg, romData, romLen, jobItem, flyBusy, flyBattle, &steps);
    if (r == ITEM_USED) {
      char msg[48];
      snprintf(msg, sizeof msg, "Used! It lasts %d steps.", steps);
      notice(msg);
    } else
      notice(item_reason(r));
  } else if (job == JOB_BIKE && safe) {
    enum item_result r = bike_register(core, game.cfg, romData, romLen, jobItem, flyBusy, flyBattle, &bikeOld);
    if (r == ITEM_USED) {
      gameKeys = 1u << SELECT;
      gameKeyFrames = 2;
      bikeWasRiding = fieldFlags & PK_FIELD_BIKE;
      bikeTook = false;
      job = JOB_BIKE_WAIT;
      jobFrames = 0;
    } else {
      job = JOB_NONE;
      notice(item_reason(r));
    }
  } else if (job == JOB_BIKE_WAIT && !companion) {
    // SELECT again if the game dropped it (a bike rolling at full speed ignores it), but only where taking it shows
    // for sure: the riding state flips, or the field's controls stay held.
    if (((fieldFlags & PK_FIELD_BIKE) != 0) != bikeWasRiding || (fieldFlags & PK_FIELD_LOCKED))
      bikeTook = true;
    if (++jobFrames >= 10) {
      if (bikeTook || !(fieldFlags & PK_FIELD_AVATAR) || --jobTries <= 0) {
        job = JOB_BIKE_RESTORE;
      } else {
        gameKeys = 1u << SELECT;
        gameKeyFrames = 2;
      }
      jobFrames = 0;
    }
  } else if (job == JOB_BIKE_RESTORE && ++jobFrames >= 20) {
    job = JOB_NONE;
    bike_restore(core, game.cfg, jobItem, bikeOld);
  } else if ((job == JOB_PC || job == JOB_REPEL || job == JOB_BIKE) && !companion && ++jobFrames > 120) {
    job = JOB_NONE;
    notice(item_reason(ITEM_FAILED));
  }


  if (companion) {
    pd_ui_update(&ui, &snap);
    ui.focusId = focusId;
    host.ffOn = false;
    host.hasState = false;
    memset(cvs, 0, (size_t)CW * CH * sizeof *cvs);
    pd_ui_draw(&ui, &canvas, &game, &snap, &host);
    map_cursor_sync(&ui, &game, &snap);
    focus_draw(&ui, focus_resolve(&ui));
  } else if (quickOpen) {
    draw_game();
    dim(0x55);
    struct pd_canvas qc;
    pd_canvas_init(&qc, CW, CH, ovPixels);
    const char *const *rows = flyList ? flyNames : repelList ? repelNames : NULL;
    bool drawn = true;
    if (rows)
      quick_list_draw(&qc, flyList ? "FLY" : "REPEL", rows, flyList ? flyT.count : repelKindCount, flyList ? flySel : repelSel);
    else
      drawn = quick_draw(&qc, &game, quick, quickN, quickSel);
    if (drawn) {
      quick_finish(&qc);
      blend_overlay(ovPixels, CW, CH, 1, 0, 0);
    }
  } else {
    draw_game();
    bool field = (fieldFlags & PK_FIELD_KNOWN) && !(fieldFlags & (PK_FIELD_LOCKED | PK_FIELD_BATTLE)) &&
                 (!wideSupported || pk_wide_scene() == PK_WIDE_SCENE_FIELD) && flyStage == 0;
    hudFieldFrames = field ? hudFieldFrames + 1 : 0;
    if (settings.hud && game.cfg && core->platform(core) == mPLATFORM_GBA && hudFieldFrames >= 20) {
      int steps = repel_steps(core, game.cfg);
      if (steps <= 0)
        hudState.repelFull = 0;
      else if (!hudState.repelFull || steps > hudState.repelFull)
        hudState.repelFull = hud_repel_full(steps);
      hudState.repel = steps > 0 ? steps : 0;
      hudState.pc = pcOk;
      hudState.wild = hud_wild_uncaught(&game, &snap);
      draw_hud();
    } else
      hudShown = false;
  }
  present();
}
