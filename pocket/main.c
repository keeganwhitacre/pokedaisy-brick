#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "wide_profile.h"
/* Native single-screen host for PokeDaisy. GPL-3.0, see ../NOTICE. */
#include "../3ds/desktop/pd_png.h"
#include "../app/src/main/cpp/pk_wide.h"
#include "display_fx.h"
#include "game_action.h"
#include "host_io.h"
#include "manual_states.h"
#include "pd_font.h"
#include "pd_menu.h"
#include "pd_romart.h"
#include "pd_settings.h"
#include "pd_ui.h"
#include "pd_ui_internal.h"
#include "platform_nextui.h"
#include "polished.h"
#include "recovery.h"
#include "rom_identity.h"
#include <SDL.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <math.h>
#include <mgba-util/vfs.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/core/serialize.h>
#include <mgba/internal/gb/gb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#define W 512
#define H 384
static uint32_t pixels[W * H];
static uint32_t companionPixels[W * H];
static color_t video[256 * 256];
static struct pd_canvas canvas;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture, *gameTexture, *quickTexture, *wideTexture;
static int l3Button = -1, r3Button = -1;
static uint32_t gamePixels[256 * 256];
static SDL_Rect lastGameRect;
/* Cache once per emulated frame; pause cannot advance the renderer. */
static void notice(const char *text);
static const char *root = ".";
static char displayKey[41];
static bool wideSupported, wideReady;
static bool battleVisible;
/* A state can change memory without changing the frame counter. Texture loss
 * and exposed windows also require repainting an otherwise unchanged pause. */
static uint64_t observationEpoch, repaintEpoch;
static uint64_t snapshotReads, framePresents;
static struct pocket_platform platform;
static bool forceFresh, graphicsReset, lifecycleFailure;
static struct pocket_display_options displayOptions;
static uint32_t colorLut[32768];
static SDL_Texture *effectTexture;
static bool effectFailed;
static int effectW, effectH, effectTW, effectTH, effectFilter = -1,
                                                 effectGrid = -1;
static int effectRW, effectRH, effectOX, effectOY;
static void effect_invalidate(void) {
  if (effectTexture)
    SDL_DestroyTexture(effectTexture);
  effectTexture = NULL;
  effectFailed = false;
  effectFilter = -1;
}
static void effect_cover(int tw, int th, SDL_Rect area, SDL_Rect reference) {
  if (!displayOptions.filter || effectFailed)
    return;
  int w = area.w, h = area.h;
  double scale = 1.;
  int lw, lh, ow, oh;
  SDL_RenderGetLogicalSize(renderer, &lw, &lh);
  if (lw > 0 && lh > 0 && SDL_GetRendererOutputSize(renderer, &ow, &oh) == 0) {
    scale = fmin((double)ow / lw, (double)oh / lh);
    w = (int)lround(w * scale);
    h = (int)lround(h * scale);
  }
  int rw = (int)lround(reference.w * scale), rh = (int)lround(reference.h * scale);
  int ox = (int)lround((reference.x - area.x) * scale);
  int oy = (int)lround((reference.y - area.y) * scale);
  if (!effectTexture || effectW != w || effectH != h || effectTW != tw ||
      effectTH != th || effectFilter != displayOptions.filter ||
      effectGrid != displayOptions.grid || effectRW != rw || effectRH != rh ||
      effectOX != ox || effectOY != oy) {
    effect_invalidate();
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096)
      return;
    uint32_t *mask = malloc((size_t)w * h * 4);
    if (!mask) {
      effectFailed = true;
      notice("Filter allocation failed");
      return;
    }
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++)
        mask[y * w + x] = pocket_filter_extended(
            displayOptions.filter, displayOptions.grid, x, y, ox, oy, rw, rh, tw, th);
    effectTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STATIC, w, h);
    if (!effectTexture ||
        SDL_UpdateTexture(effectTexture, NULL, mask, w * 4) < 0 ||
        SDL_SetTextureBlendMode(effectTexture, SDL_BLENDMODE_BLEND) < 0) {
      effect_invalidate();
      effectFailed = true;
      notice("Filter unavailable on this video backend");
    }
    free(mask);
    effectW = w;
    effectH = h;
    effectTW = tw;
    effectTH = th;
    effectFilter = displayOptions.filter;
    effectGrid = displayOptions.grid;
    effectRW = rw; effectRH = rh; effectOX = ox; effectOY = oy;
  }
  if (effectTexture &&
      SDL_RenderCopy(renderer, effectTexture, NULL, &area) < 0)
    notice("Filter draw failed");
}
static void effect_draw(int tw, int th) {
  effect_cover(tw, th, lastGameRect, lastGameRect);
}
static void options_save(void) {
  if (!pocket_options_save(root, displayKey, &displayOptions))
    notice("Settings could not be saved");
}
static int wideMode; /* 0 original, 1 expand, 2 zoom. */
static unsigned wideW, wideH;
static bool wideGba;
static uint32_t wideFrame[320 * 240], widePixels[320 * 240], gbFrame[240 * 160];
static uint32_t fadeWorld[320 * 240];
static unsigned fadeW, fadeH, fadeStep;
static bool fadeArmed, fadeActive, fadeHaveWorld;
static void fade_reset(void) {
  fadeArmed = fadeActive = fadeHaveWorld = false; fadeStep = 0;
  memset(fadeWorld, 0, sizeof fadeWorld);
}
static void wide_tick(struct mCore *c, bool skip) {
  if (!wideSupported)
    return;
  if (wideGba) {
    for (int y = 0; y < 160; y++)
      memcpy(gbFrame + y * 240, video + y * 256, 240 * sizeof *gbFrame);
    wideW = wideMode == 2 ? 320 : 240;
    wideH = wideMode == 2 ? 240 : 180;
    struct PkWideView v = {(240 - (int)wideW) / 2,
                          (160 - (int)wideH) / 2, (int)wideW, (int)wideH};
    if (skip || !wideMode) {
      pk_wide_skip(c, gbFrame);
      wideReady = false;
      return;
    }
    int mix = pk_wide_view(c, gbFrame, wideFrame, v, NULL);
    int scene = pk_wide_scene();
    wideReady = mix == 256 && pk_wide_last_mode() == PK_WIDE_MODE_MAP &&
                !pk_wide_fault() && (scene == PK_WIDE_SCENE_FIELD ||
                (!displayOptions.autoZoom && scene == PK_WIDE_SCENE_TEXT));
    fadeActive = fadeArmed = fadeHaveWorld = false;
#ifdef POCKET_TEST
    if (wideReady)
      for (int y = 0; y < 160; y++)
        for (int x = 0; x < 240; x++)
          if (wideFrame[(y - v.y) * wideW + x - v.x] != gbFrame[y * 240 + x])
            abort();
#endif
    return;
  }
  for (int y = 0; y < 144; y++)
    for (int x = 0; x < 160; x++)
      gbFrame[y * 160 + x] = video[y * 256 + x];
  wideW = wideMode == 1 ? 204 : 256;
  wideH = wideMode == 1 ? 152 : 192;
  struct PkWideView v = {(160 - (int)wideW) / 2, (144 - (int)wideH) / 2,
                         (int)wideW, (int)wideH};
  if (skip || !wideMode) {
    pk_wide_gb_view(c, gbFrame, NULL, v, NULL);
    wideReady = false;
    return;
  }
  int mix = pk_wide_gb_view(c, gbFrame, wideFrame, v, NULL);
  int scene = pk_wide_gb_scene();
  wideReady = mix == 256 && pk_wide_gb_last_mode() == PK_WIDE_MODE_MAP &&
              (scene == PK_WIDE_SCENE_FIELD ||
               (!displayOptions.autoZoom && scene == PK_WIDE_SCENE_TEXT));
  if (wideReady) {
    fadeW = wideW; fadeH = wideH;
    fadeArmed = true; fadeActive = false; fadeStep = 0;
  } else if (!fadeActive && fadeArmed && fadeHaveWorld && scene == PK_WIDE_SCENE_BATTLE_START) {
    fadeActive = true; fadeArmed = false; fadeStep = 1;
  } else if (fadeActive && ++fadeStep >= 10) {
    fadeActive = false;
  }
#ifdef POCKET_TEST
  /* Pixels in the actual PPU viewport must survive expansion unchanged. */
  if (wideReady)
    for (int y = 0; y < 144; y++)
      for (int x = 0; x < 160; x++)
        if (wideFrame[(y + (wideH - 144) / 2) * wideW + x +
                      (wideW - 160) / 2] != gbFrame[y * 160 + x])
          abort();
#endif
}
static void wide_cycle(void) {
  fade_reset();
  if (displayOptions.stretch) {
    displayOptions.stretch = false;
    options_save();
  }
  if (!wideSupported) {
    notice("Expansion unavailable for this ROM build");
    return;
  }
  wideMode = (wideMode + 1) % 3;
  wideReady = false;
  if (!pocket_display_save(root, displayKey, wideMode)) {
    notice("Display preference could not be saved");
    return;
  }
  notice(wideMode == 1   ? "Expand"
         : wideMode == 2 ? "Zoom out"
                         : "Original display");
}
static SDL_AudioDeviceID audio;
static bool audioPaused;
static volatile sig_atomic_t running = 1;
static bool muted;
static int appError;
#ifdef POCKET_TEST
static void *test_no_map(struct VFile *vf, size_t size, int flags) {
  (void)vf;
  (void)size;
  (void)flags;
  return NULL;
}
static unsigned audioNonzero;
static bool testQuick, testFf;
static int testQuickSelected, testMenu;
#endif
static char message[128];
static unsigned messageUntil;
enum {
  A,
  B,
  SELECT,
  START,
  RIGHT,
  LEFT,
  UP,
  DOWN,
  R,
  L,
  X,
  Y,
  L2,
  R2,
  MENU,
  L3,
  R3,
  NKEYS
};
static unsigned held, pressed;
static unsigned repeatAt[NKEYS];
static void stop(int sig) {
  (void)sig;
  running = 0;
}
static void notice(const char *s) {
  repaintEpoch++;
  snprintf(message, sizeof message, "%s", s);
  messageUntil = SDL_GetTicks() + 2500;
  fprintf(stderr, "%s\n", s);
}
static void post_audio(struct mAVStream *s, blip_t *l, blip_t *r) {
  (void)s;
  int16_t out[2048];
  int n = blip_samples_avail(l);
  if (n > 1024)
    n = 1024;
  blip_read_samples(l, out, n, true);
  blip_read_samples(r, out + 1, n, true);
#ifdef POCKET_TEST
  for (int i = 0; i < n * 2; i++)
    if (out[i])
      audioNonzero++;
#endif
  if (audio && !muted && SDL_GetQueuedAudioSize(audio) < 32768)
    SDL_QueueAudio(audio, out, n * 4);
}
static bool read_memory(void *ctx, uint32_t a, void *out, size_t n) {
  struct mCore *c = ctx;
  unsigned char *p = out;
  if (c->platform(c) == mPLATFORM_GB && a >= 0x10000) {
    unsigned bank = a >> 16, addr = a & 65535;
    struct GB *gb = c->board;
    if (bank > 7 || addr < 0xd000 || addr >= 0xe000 || n > 0xe000 - addr ||
        !gb->memory.wram)
      return false;
    memcpy(p, gb->memory.wram + bank * 0x1000 + addr - 0xd000, n);
    return true;
  }
  if (c->platform(c) == mPLATFORM_GBA && a >= 0x08000000 && a < 0x0e000000) {
    size_t size = 0;
    const unsigned char *r = c->getMemoryBlock(c, 0x08, &size);
    size_t start = (a - 0x08000000) & 0x01ffffff;
    if (!r || start > size || n > size - start)
      return false;
    memcpy(p, r + start, n);
    return true;
  }
  for (size_t i = 0; i < n; i++)
    p[i] = c->busRead8(c, a + (uint32_t)i);
  return true;
}
static int keyboard(SDL_Keycode k) {
  switch (k) {
  case SDLK_z:
    return A;
  case SDLK_x:
    return B;
  case SDLK_BACKSPACE:
    return SELECT;
  case SDLK_RETURN:
    return START;
  case SDLK_RIGHT:
    return RIGHT;
  case SDLK_LEFT:
    return LEFT;
  case SDLK_UP:
    return UP;
  case SDLK_DOWN:
    return DOWN;
  case SDLK_e:
    return R;
  case SDLK_q:
    return L;
  case SDLK_TAB:
    return X;
  case SDLK_f:
    return Y;
  case SDLK_1:
    return L2;
  case SDLK_2:
    return R2;
  case SDLK_F3:
    return L3;
  case SDLK_F4:
    return R3;
  case SDLK_ESCAPE:
    return MENU;
  default:
    return -1;
  }
}
static int joy(int n) {
  if (n == l3Button)
    return L3;
  if (n == r3Button)
    return R3;
  static const int map[] = {B, A, Y, X, L, R, SELECT, START, MENU, -1, L2, R2};
  return n >= 0 && n < 12 ? map[n] : -1;
}
static void key(int k, bool down) {
  if (k < 0)
    return;
  unsigned bit = 1u << k;
  if (down) {
    if (!(held & bit)) {
      pressed |= bit;
      repeatAt[k] = SDL_GetTicks() + 400;
    }
    held |= bit;
  } else
    held &= ~bit;
}
static void events(void) {
  pressed = 0;
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT)
      running = 0;
    if (e.type == SDL_RENDER_TARGETS_RESET || e.type == SDL_RENDER_DEVICE_RESET)
      graphicsReset = true;
    if (e.type == SDL_WINDOWEVENT &&
        (e.window.event == SDL_WINDOWEVENT_EXPOSED ||
         e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
         e.window.event == SDL_WINDOWEVENT_RESTORED))
      repaintEpoch++;
    if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
      if (e.key.keysym.scancode == SDL_SCANCODE_POWER ||
          e.key.keysym.sym == SDLK_POWER) {
        if (!e.key.repeat && !platform.count)
          pocket_platform_key(&platform, e.type == SDL_KEYDOWN, SDL_GetTicks());
        continue;
      }
      if (!e.key.repeat)
        key(keyboard(e.key.keysym.sym), e.type == SDL_KEYDOWN);
    }
    if (e.type == SDL_JOYBUTTONDOWN || e.type == SDL_JOYBUTTONUP)
      key(joy(e.jbutton.button), e.type == SDL_JOYBUTTONDOWN);
    if (e.type == SDL_JOYHATMOTION) {
      key(UP, e.jhat.value & SDL_HAT_UP);
      key(DOWN, e.jhat.value & SDL_HAT_DOWN);
      key(LEFT, e.jhat.value & SDL_HAT_LEFT);
      key(RIGHT, e.jhat.value & SDL_HAT_RIGHT);
    }
    if (e.type == SDL_JOYAXISMOTION) {
      if (e.jaxis.axis == 2)
        key(L2, e.jaxis.value > 16000);
      if (e.jaxis.axis == 5)
        key(R2, e.jaxis.value > 16000);
    }
    if (e.type == SDL_WINDOWEVENT &&
        e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
      held = 0;
  }
  pocket_platform_poll(&platform, SDL_GetTicks());
}
static void repeat_directions(void) {
  for (int k = RIGHT; k <= DOWN; k++)
    if ((held & (1u << k)) && SDL_GetTicks() >= repeatAt[k]) {
      pressed |= 1u << k;
      repeatAt[k] = SDL_GetTicks() + 90;
    }
}
static bool down(int k) { return (pressed & (1u << k)) != 0; }
static void present_ui(bool companion) {
  framePresents++;
  SDL_RenderSetLogicalSize(renderer, W, H);
  if (SDL_GetTicks() < messageUntil) {
    pd_fill(&canvas, 4, 334, 504, 24, 0x181818);
    pd_text_fit(&canvas, 10, 338, 492, message, 0xffffff, PD_NO_SHADOW);
  }
  const uint32_t *upload = pixels;
  bool effects = companion && displayOptions.companionShaders;
  if (effects && displayOptions.colors) {
    for (int i = 0; i < W * H; i++)
      companionPixels[i] = pocket_color_apply(colorLut, pixels[i]);
    upload = companionPixels;
  }
  if (SDL_UpdateTexture(texture, NULL, upload, W * 4) < 0) {
    notice(SDL_GetError());
    return;
  }
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0) {
    notice(SDL_GetError());
    return;
  }
  if (effects) {
    SDL_Rect saved = lastGameRect;
    lastGameRect = (SDL_Rect){0, 0, W, H};
    /* ROM art uses two logical canvas pixels per original art pixel. */
    effect_draw(W / 2, H / 2);
    lastGameRect = saved;
  }
  SDL_RenderPresent(renderer);
}
static void present(void) { present_ui(false); }
/* Fill only the space outside the original PPU image. Matching light corners
 * and a mostly uniform top border reject dark transitions/menus/animations.
 * Use this frame's actual palette, never a guessed white or cached terrain. */
static uint32_t battle_background(unsigned gw, unsigned gh) {
  if (!battleVisible || !displayOptions.battleBackground || gw != 160 ||
      gh != 144 || wideReady)
    return 0;
  uint32_t color = video[0] & 0xffffff;
  if ((color & 255) < 128 || ((color >> 8) & 255) < 128 ||
      ((color >> 16) & 255) < 128 ||
      (video[159] & 0xffffff) != color ||
      (video[143 * 256] & 0xffffff) != color ||
      (video[143 * 256 + 159] & 0xffffff) != color)
    return 0;
  int matching = 0;
  for (int x = 0; x < 160; x++)
    matching += (video[x] & 0xffffff) == color;
  if (matching < 120)
    return 0;
  uint32_t rgb = ((color & 255) << 16) | (color & 0xff00) | (color >> 16);
  return displayOptions.colors ? pocket_color_apply(colorLut, rgb) : rgb;
}
// Gameplay scales in physical panel pixels; the companion keeps its 512x384
// grid.
static void render_game(unsigned gw, unsigned gh) {
  framePresents++;
  int ow, oh;
  if (SDL_GetRendererOutputSize(renderer, &ow, &oh) < 0 || ow <= 0 || oh <= 0)
    return;
  SDL_RenderSetLogicalSize(renderer, ow, oh);
  int scale = ow / (int)gw;
  if (oh / (int)gh < scale)
    scale = oh / (int)gh;
  if (scale < 1)
    scale = 1;
  lastGameRect =
      (SDL_Rect){(ow - (int)gw * scale) / 2, (oh - (int)gh * scale) / 2,
                 (int)gw * scale, (int)gh * scale};
  SDL_Texture *drawTexture = gameTexture;
  if (wideReady && !displayOptions.stretch) {
    gw = wideW;
    gh = wideH;
    drawTexture = wideTexture;
    scale = ow / (int)gw;
    if (oh / (int)gh < scale)
      scale = oh / (int)gh;
    if (scale < 1)
      scale = 1;
    lastGameRect =
        (SDL_Rect){(ow - (int)gw * scale) / 2, (oh - (int)gh * scale) / 2,
                   (int)gw * scale, (int)gh * scale};
    for (unsigned i = 0; i < gw * gh; i++) {
      uint32_t n = wideFrame[i];
      widePixels[i] = ((n & 255) << 16) | (n & 0xff00) | ((n >> 16) & 255);
      if (displayOptions.colors)
        widePixels[i] = pocket_color_apply(colorLut, widePixels[i]);
    }
    /* Keep the last visible world through the game's black transition frames.
     * No guessed terrain: these are pixels already shown by this renderer. */
    unsigned brightness = 0, visible = 0, distinct = 0, firstMatches = 0;
    uint32_t colors[3] = {0};
    for (unsigned k = 0; k < 32; k++) {
      uint32_t p = gbFrame[k * 160 * 144 / 32];
      p &= 0xffffff;
      bool found = false;
      for (unsigned j = 0; j < distinct; j++) found |= colors[j] == p;
      if (!found && distinct < 3) colors[distinct++] = p;
      firstMatches += p == colors[0];
      unsigned light = (p & 255) + ((p >> 8) & 255) + ((p >> 16) & 255);
      brightness += light;
      visible += light > 96;
    }
    if (!wideGba && pk_wide_gb_scene() == PK_WIDE_SCENE_FIELD && distinct == 3 &&
        firstMatches <= 24 && visible >= 16 && brightness > 32 * 3 * 16) {
      memcpy(fadeWorld, widePixels, gw * gh * sizeof *fadeWorld);
      fadeHaveWorld = true;
    }
    if (SDL_UpdateTexture(wideTexture, &(SDL_Rect){0, 0, (int)gw, (int)gh},
                          widePixels, gw * 4) < 0) {
      notice(SDL_GetError());
      return;
    }
  } else {
    for (unsigned y = 0; y < gh; y++)
      for (unsigned x = 0; x < gw; x++) {
        uint32_t n = video[y * 256 + x];
        gamePixels[y * 256 + x] =
            ((n & 255) << 16) | (n & 0xff00) | ((n >> 16) & 255);
        if (displayOptions.colors)
          gamePixels[y * 256 + x] =
              pocket_color_apply(colorLut, gamePixels[y * 256 + x]);
      }
    if (SDL_UpdateTexture(gameTexture, NULL, gamePixels, 256 * 4) < 0) {
      notice(SDL_GetError());
      return;
    }
  }
  uint32_t background = battle_background(gw, gh);
  SDL_SetRenderDrawColor(renderer, background >> 16, background >> 8,
                         background, 255);
  SDL_RenderClear(renderer);
  if (displayOptions.stretch)
    lastGameRect = (SDL_Rect){0, 0, ow, oh};
  SDL_Rect src = {0, 0, (int)gw, (int)gh};
  if (fadeActive && !wideReady && !displayOptions.stretch && fadeW && fadeH) {
    SDL_Rect fs = {0, 0, (int)fadeW, (int)fadeH};
    int z = ow / (int)fadeW;
    if (oh / (int)fadeH < z) z = oh / (int)fadeH;
    if (z < 1) z = 1;
    SDL_Rect fd = {(ow - (int)fadeW * z) / 2, (oh - (int)fadeH * z) / 2,
                   (int)fadeW * z, (int)fadeH * z};
    if (!SDL_UpdateTexture(wideTexture, &fs, fadeWorld, fadeW * 4)) {
      SDL_RenderCopy(renderer, wideTexture, &fs, &fd);
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(fadeStep * 255 / 10));
      SDL_RenderFillRect(renderer, NULL);
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
  }
  if (SDL_RenderCopy(renderer, drawTexture, &src, &lastGameRect) < 0) {
    notice(SDL_GetError());
    return;
  }
  if ((background || fadeActive) && !displayOptions.stretch)
    effect_cover(gw, gh, (SDL_Rect){0, 0, ow, oh}, lastGameRect);
  else
    effect_draw(gw, gh);
  /* The caller may draw a quick-menu overlay before presenting. */
}
static void present_game(unsigned gw, unsigned gh) {
  render_game(gw, gh);
  SDL_RenderPresent(renderer);
}
#include "quick_menu.inc"
static bool capture_frame(const char *path, unsigned *lit) {
  int ow, oh;
  if (SDL_GetRendererOutputSize(renderer, &ow, &oh) < 0 || ow <= 0 || oh <= 0 ||
      ow > 8192 || oh > 8192)
    return false;
  uint32_t *shot = malloc((size_t)ow * oh * 4);
  if (!shot)
    return false;
  bool ok = SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGB888, shot,
                                 ow * 4) == 0;
  if (ok && lit) {
    *lit = 0;
    for (int i = 0; i < ow * oh; i++)
      if (shot[i] & 0xffffff)
        (*lit)++;
  }
  if (ok && path)
    ok = pd_png_write(path, ow, oh, shot);
  free(shot);
  return ok;
}
static bool copy_backup(const char *path) {
  FILE *in = fopen(path, "rb");
  if (!in)
    return errno == ENOENT;
  char dest[1200];
  snprintf(dest, sizeof dest, "%s.backup-XXXXXX", path);
  int fd = mkstemp(dest);
  FILE *out = fd >= 0 ? fdopen(fd, "wb") : NULL;
  if (!out) {
    if (fd >= 0)
      close(fd);
    fclose(in);
    return false;
  }
  char b[8192];
  size_t n;
  bool ok = true;
  while ((n = fread(b, 1, sizeof b, in)))
    if (fwrite(b, 1, n, out) != n) {
      ok = false;
      break;
    }
  if (ferror(in))
    ok = false;
  fclose(in);
  if (fclose(out))
    ok = false;
  return ok;
}
static bool state(struct mCore *c, const char *path, bool save) {
  if (strlen(displayKey) == 40) {
    bool ok = save ? pocket_slot_save(c, path, displayKey)
                   : pocket_slot_load(c, path, displayKey);
    if (ok && !save) {
      fade_reset();
      observationEpoch++;
      pk_wide_reset();
      wideReady = false;
    }
    return ok;
  }
  char tmp[1200];
  snprintf(tmp, sizeof tmp, "%s.tmp", path);
  struct VFile *f = VFileOpen(save ? tmp : path,
                              save ? O_CREAT | O_TRUNC | O_RDWR : O_RDONLY);
  if (!f)
    return false;
  bool ok = save ? mCoreSaveStateNamed(c, f, SAVESTATE_RTC)
                 : mCoreLoadStateNamed(c, f, SAVESTATE_RTC);
  f->close(f);
  if (ok && !save) {
    fade_reset();
    observationEpoch++;
    pk_wide_reset();
    wideReady = false;
  }
  if (save) {
    if (ok)
      ok = rename(tmp, path) == 0;
    else
      unlink(tmp);
  }
  return ok;
}
static void audio_pause(bool pause) {
  if (audio && pause != audioPaused) {
    if (pause)
      SDL_ClearQueuedAudio(audio);
    SDL_PauseAudioDevice(audio, pause);
    audioPaused = pause;
  }
}
static void audio_open(void) {
  SDL_AudioSpec want = {0};
  want.freq = 32768;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  want.samples = 512;
  audio = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
  audioPaused = false;
  if (audio)
    SDL_PauseAudioDevice(audio, 0);
  else
    fprintf(stderr, "Audio unavailable: %s\n", SDL_GetError());
}
static bool textures_reset(void) {
  repaintEpoch++;
  effect_invalidate();
  SDL_DestroyTexture(quickTexture);
  quickTexture = NULL;
  SDL_DestroyTexture(texture);
  SDL_DestroyTexture(gameTexture);
  SDL_DestroyTexture(wideTexture);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                              SDL_TEXTUREACCESS_STREAMING, W, H);
  gameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING, 256, 256);
  wideTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING, 320, 240);
  graphicsReset = false;
  return texture && gameTexture && wideTexture;
}
#ifdef POCKET_TEST
static bool (*test_suspend)(void);
#endif
/* Foreground thread only, between emulated frames. The suspend helper owns
 * the real hardware transition; the core and CPU expansion cache stay still. */
static bool lifecycle(struct mCore *c, const char *battery,
                      struct pocket_action *action, uint64_t *due) {
  if (graphicsReset && !textures_reset()) {
    notice("Video reset failed");
    running = 0;
    return true;
  }
  if (!platform.powerDown && !platform.sleepRequested &&
      !platform.shutdownRequested)
    return false;
  held = pressed = 0;
  memset(repeatAt, 0, sizeof repeatAt);
  if (c)
    c->setKeys(c, 0);
  if (action)
    pocket_action_cancel(action);
  if (audio) {
    audio_pause(true);
    SDL_ClearQueuedAudio(audio);
  }
  if (platform.shutdownRequested) {
    running = 0;
    return true;
  }
  if (platform.powerDown) {
    SDL_Delay(16);
    return true;
  }
  if (c && displayOptions.autoResume &&
      !pocket_recovery_save(c, root, displayKey, battery))
    notice("Recovery could not be saved before sleep");
  if (audio) {
    SDL_CloseAudioDevice(audio);
    audio = 0;
  }
  bool ok;
#ifdef POCKET_TEST
  if (test_suspend)
    ok = test_suspend();
  else
#endif
    ok = pocket_platform_suspend(&platform);
  pocket_platform_woke(&platform, SDL_GetTicks());
  SDL_FlushEvents(SDL_KEYDOWN, SDL_KEYUP);
  SDL_FlushEvents(SDL_JOYAXISMOTION, SDL_JOYBUTTONUP);
  SDL_FlushEvents(SDL_CONTROLLERAXISMOTION, SDL_CONTROLLERBUTTONUP);
  SDL_JoystickUpdate();
  if (!textures_reset()) {
    notice("Video restore failed");
    running = 0;
  }
  audio_open();
  if (due)
    *due = SDL_GetPerformanceCounter();
  if (!ok) {
    notice("Suspend failed or unsupported; gameplay restored");
    lifecycleFailure = true;
  }
  return true;
}
static void focus_move(struct pd_ui *u, int *focus, int direction) {
  if (!u->hitCount) {
    *focus = -1;
    return;
  }
  if (*focus < 0 || *focus >= u->hitCount) {
    *focus = 0;
    return;
  }
  const struct pd_hit *h = &u->hits[*focus];
  int best = -1, score = 1000000;
  for (int i = 0; i < u->hitCount; i++) {
    if (i == *focus)
      continue;
    struct pd_hit *t = &u->hits[i];
    if (t->id == HIT_SCRIM || t->id == HIT_BAG_LIST || t->id == HIT_DEX_LIST ||
        t->id == HIT_GUIDE_LIST)
      continue;
    int dx = (t->x + t->w / 2) - (h->x + h->w / 2),
        dy = (t->y + t->h / 2) - (h->y + h->h / 2);
    int along = direction == LEFT    ? -dx
                : direction == RIGHT ? dx
                : direction == UP    ? -dy
                                     : dy;
    int across = direction == LEFT || direction == RIGHT ? abs(dy) : abs(dx);
    if (along > 0 && along + across * 4 < score) {
      best = i;
      score = along + across * 4;
    }
  }
  if (best >= 0)
    *focus = best;
  else if (direction == UP || direction == DOWN) {
    int d = direction == DOWN ? 36 : -36;
    if (u->overlay == PD_OVERLAY_PLACES)
      u->placesScroll += d;
    else if (u->tab == PD_TAB_BAG)
      u->bagScroll += d;
    else if (u->tab == PD_TAB_DEX)
      u->dexScroll += d;
    else if (u->tab == PD_TAB_GUIDE)
      u->guideScroll += d;
  }
}
static void focus_draw(struct pd_ui *u, int focus) {
  if (focus < 0 || focus >= u->hitCount)
    return;
  struct pd_hit *h = &u->hits[focus];
  pd_fill(&canvas, h->x, h->y, h->w, 2, 0xff7331);
  pd_fill(&canvas, h->x, h->y + h->h - 2, h->w, 2, 0xff7331);
  pd_fill(&canvas, h->x, h->y, 2, h->h, 0xff7331);
  pd_fill(&canvas, h->x + h->w - 2, h->y, 2, h->h, 0xff7331);
}
#include "state_browser.inc"
#ifdef POCKET_TEST
static void test_frame(struct mCore *, int *, bool, int, struct pd_ui *,
                       struct pd_snapshot *, const struct pc_ui *);
#endif
static void run_game(const char *path, int maxFrames, const char *shot) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    appError = 1;
    notice("Cannot open ROM");
    return;
  }
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  rewind(f);
  if (len < 0x150 || len > 40 * 1024 * 1024) {
    fclose(f);
    appError = 1;
    notice("Invalid ROM size");
    return;
  }
  unsigned char *romdata = malloc((size_t)len);
  if (!romdata || fread(romdata, 1, len, f) != (size_t)len) {
    free(romdata);
    fclose(f);
    notice("Cannot read ROM");
    return;
  }
  fclose(f);
  struct VFile *vf = VFileOpen(path, O_RDONLY);
  struct mCore *c = vf ? mCoreFindVF(vf) : NULL;
  if (!c || !c->init(c)) {
    if (vf)
      vf->close(vf);
    free(romdata);
    appError = 1;
    notice("Unsupported ROM");
    return;
  }
  mCoreInitConfig(c, NULL);
  if (c->platform(c) == mPLATFORM_GB) {
    // Match upstream's Android GB setup, including SGB-enhanced cartridges.
    mCoreConfigSetIntValue(&c->config, "sgb.borders", 0);
    c->reloadConfigOption(c, "sgb.borders", &c->config);
  }
  c->setVideoBuffer(c, video, 256);
  c->setAudioBufferSize(c, 512);
  if (!c->loadROM(c, vf)) {
    c->deinit(c);
    free(romdata);
    notice("ROM load failed");
    return;
  }
  struct pocket_save_status saveStatus = {0};
  struct VFile *sv = NULL;
  bool backupMade = false;
  char sav[1024], ss[1024];
  const char *name = strrchr(path, '/');
  name = name ? name + 1 : path;
  snprintf(sav, sizeof sav, "%s/saves/%s.sav", root, name);
  snprintf(ss, sizeof ss, "%s/states/%s.ss", root, name);
  bool hadSave = access(sav, F_OK) == 0;
  if (!copy_backup(sav)) {
    notice("Save backup failed; game not opened");
    goto cleanup;
  }
  backupMade = hadSave;
  struct VFile *rawSave = VFileOpen(sav, O_CREAT | O_RDWR);
#ifdef POCKET_TEST
  if (rawSave && getenv("POCKET_TEST_NO_MMAP"))
    rawSave->map = test_no_map;
#endif
  sv = pocket_save_wrap(rawSave, &saveStatus);
  if (!sv || !c->loadSave(c, sv) || saveStatus.failed) {
    notice("Save file unavailable; game not opened");
    goto cleanup;
  }
  struct mAVStream stream = {.postAudioBuffer = post_audio};
  c->setAVStream(c, &stream);
  for (int i = 0; i < 2; i++)
    blip_set_rates(c->getAudioChannel(c, i), c->frequency(c), 32768);
  c->reset(c);
  uint32_t wideCrc = 0;
  wideGba = c->platform(c) == mPLATFORM_GBA;
  c->checksum(c, &wideCrc, mCHECKSUM_CRC32);
  wideSupported = wideGba ? pk_wide_configure(wideCrc)
                          : pocket_wide_configure(wideCrc);
  wideReady = false;
  pocket_rom_sha1(romdata, (size_t)len, displayKey);
  wideMode = wideSupported ? pocket_display_load(root, displayKey) : 0;
  unsigned gw, gh;
  c->desiredVideoDimensions(c, &gw, &gh);
  struct pd_game game;
  pd_game_detect(&game, romdata, (size_t)len);
  game.rom = romdata;
  game.romSize = (size_t)len;
  int pc =
      c->platform(c) == mPLATFORM_GB ? pc_identify(romdata, (size_t)len) : -1;
  if (c->platform(c) == mPLATFORM_GBA)
    pd_romart_scan(&game);
  char config[1024];
  snprintf(config, sizeof config, "%s/settings.ini", root);
  pocket_options_load(root, displayKey, &displayOptions);
  if (displayOptions.autoResume && !forceFresh) {
    enum pocket_recovery_result result =
        pocket_recovery_load(c, root, displayKey, sav);
    if (result == RECOVERY_LOADED)
      notice("Session restored");
    else if (result == RECOVERY_INVALID)
      notice("Invalid recovery ignored; starting normally");
    else if (result == RECOVERY_STALE)
      notice("Battery save changed; recovery ignored");
  }
  pocket_color_lut(colorLut);
  effect_invalidate();
  struct pd_settings settings;
  pd_settings_load(&settings, config);
  settings.ffSpeed = displayOptions.ffSpeed;
  battleVisible = false;
  fade_reset();
  struct pd_ui ui;
  pd_ui_init(&ui, &settings);
  struct pd_snapshot snap = {0};
  struct pd_host_info host = {.hideScreenMode = true, .count = 1, .labels = {"HOST"}};
  snprintf(host.values[0], 40, "PokeDaisy Pocket");
  const char *opts[] = {"Resume game",
                        "Party",
                        "Save state",
                        "Load state",
                        "Toggle fast-forward",
                        "Close game",
                        "About",
                        "Display: Original / Expand / Zoom",
                        "Quick menu",
                        "Settings"};
  const char *confirmOpts[] = {"Cancel", "Confirm"};
  struct pd_menu gameMenu, confirmMenu;
  pd_menu_init(&gameMenu, "GAME", opts, 10);
  pd_menu_init(&confirmMenu, "CONFIRM", confirmOpts, 2);
  char optionText[10][80];
  const char *optionLabels[10];
  for (int i = 0; i < 10; i++)
    optionLabels[i] = optionText[i];
  struct pd_menu screenMenu;
  pd_menu_init(&screenMenu, "SCREEN / SESSION", optionLabels, 10);
  int menuOpen = 0;
  int slotIndex = 0;
  bool slotSave = true, slotConfirm = false;
  struct pocket_slot *slots = NULL;
  char slotPath[1200];
  bool quickOpen = false;
  int quickSelected = 0;
  bool companion = false, ff = false, leave = false;
  int focus = 0, frames = 0;
  struct pc_ui pcUi = {.battleSupported = pocket_battle_supported(displayKey)};
  pocket_favorites_load(root, displayKey, &pcUi.favorites);
  struct pocket_action gameAction = {0};
  struct pocket_battle_action battleAction = {0};
  bool battleReturn = false;
  const char *idleSetting = getenv("POCKET_IDLE_CACHE");
  bool cacheIdle = !idleSetting || strcmp(idleSetting, "0");
  bool snapshotValid = false, painted = false;
  unsigned snapshotFrame = 0;
  uint64_t snapshotEpoch = observationEpoch, paintedEpoch = 0;
  bool paintedCompanion = false, paintedQuick = false, paintedNotice = false;
  int paintedMenu = -1;
  unsigned paintedAnimation = 0, paintedMessageUntil = 0, paintedDisplay = 0;
  snapshotReads = framePresents = 0;
  uint64_t due = SDL_GetPerformanceCounter();
  double hz = (double)c->frequency(c) / c->frameCycles(c);
  struct timespec cpuStart = {0}, wallStart = {0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpuStart);
  clock_gettime(CLOCK_MONOTONIC, &wallStart);
#ifndef POCKET_TEST
play_loop:
#endif
  while (running && !leave) {
    events();
    if (!running)
      break;
    if (platform.powerDown || platform.sleepRequested ||
        platform.shutdownRequested) {
      pocket_battle_cancel(&battleAction);
      battleReturn = false;
    }
    if (lifecycle(c, sav, &gameAction, &due)) {
      if (lifecycleFailure) {
        lifecycleFailure = false;
        companion = true;
        menuOpen = 1;
        quickOpen = false;
      }
      continue;
    }
    repeat_directions();
    bool menuRequested =
        down(MENU) || ((down(START) || down(SELECT)) &&
                       (held & (1u << START)) && (held & (1u << SELECT)));
    if ((battleAction.result == PA_RUNNING || battleReturn) &&
        (down(B) || down(X) || menuRequested || down(L3))) {
      pocket_battle_cancel(&battleAction);
      battleReturn = false;
      c->setKeys(c, 0);
      held &= ~1023u;
      notice(pocket_battle_message(&battleAction));
    }
    if (gameAction.result == PA_RUNNING &&
        (down(B) || down(X) || menuRequested || down(L3))) {
      pocket_action_cancel(&gameAction);
      c->setKeys(c, 0);
      held &= ~1023u;
      notice(pocket_action_message(&gameAction));
    }
    bool favoriteRequested = down(X) && companion && !menuOpen && !quickOpen &&
                             pc >= 0 && pcUi.section == PC_BAG && pcUi.detail;
    if (down(X) && !favoriteRequested) {
      quickOpen = false;
      menuOpen = 0;
      companion = !companion;
      c->setKeys(c, 0);
      if (audio)
        SDL_ClearQueuedAudio(audio);
    }
    if (menuRequested) {
      quickOpen = false;
      menuOpen = menuOpen ? 0 : 1;
      companion = true;
      c->setKeys(c, 0);
      if (audio)
        SDL_ClearQueuedAudio(audio);
    }
    if (down(L3)) {
      quickOpen = !quickOpen;
      if (!quickOpen) {
        held &= ~1023u;
        c->setKeys(c, 0);
      }
      if (quickOpen) {
        quickSelected = 0;
        c->setKeys(c, 0);
        if (audio)
          SDL_ClearQueuedAudio(audio);
      }
    }
    if (down(R3) && !companion && !quickOpen)
      wide_cycle();
    if (down(Y) && !companion && !quickOpen)
      ff = !ff;
    if (quickOpen) {
      muted = true;
      if (down(B))
        quickOpen = false;
      else {
        if (down(UP))
          quickSelected = 0;
        if (down(RIGHT))
          quickSelected = 1;
        if (down(DOWN))
          quickSelected = 2;
        if (down(LEFT))
          quickSelected = 3;
        if (down(A)) {
          if (quickSelected == 2) {
            ff = !ff;
            quickOpen = false;
          }
          if (quickSelected == 3) {
            companion = true;
            menuOpen = 0;
            quickOpen = false;
          }
          if (quickSelected == 0 ||
              (quickSelected == 1 && access(ss, F_OK) == 0)) {
            companion = true;
            menuOpen = quickSelected == 0 ? 5 : 2;
            confirmMenu.selected = 0;
            confirmMenu.title =
                quickSelected == 0
                    ? "Overwrite save state?"
                    : "Load state? Current progress will rewind.";
            quickOpen = false;
          }
        }
      }
      if (!quickOpen)
        held &= ~1023u;
    } else if (companion) {
      if (menuOpen) {
        if (menuOpen == 7) {
          if (!slots) {
            slots = calloc(10, sizeof *slots);
            if (slots)
              slots_refresh(c, ss, slots);
          }
          int previousSlot = slotIndex;
          if (down(LEFT) && slotIndex % 2)
            slotIndex--;
          if (down(RIGHT) && !(slotIndex % 2))
            slotIndex++;
          if (down(UP) && slotIndex >= 2)
            slotIndex -= 2;
          if (down(DOWN) && slotIndex < 8)
            slotIndex += 2;
          if (previousSlot != slotIndex)
            slotConfirm = false;
          if (down(Y)) {
            slotSave = !slotSave;
            slotConfirm = false;
          }
          if (down(B)) {
            if (slotConfirm)
              slotConfirm = false;
            else {
              menuOpen = 0;
              companion = false;
            }
          } else if (down(A) && slots) {
            pocket_slot_path(ss, slotIndex, slotPath);
            bool occupied = slots[slotIndex].status != SLOT_EMPTY;
            if (!slotSave && (slots[slotIndex].status == SLOT_EMPTY ||
                              slots[slotIndex].status == SLOT_INVALID))
              notice(occupied ? "State unavailable or damaged"
                              : "This slot is empty");
            else if ((slotSave && occupied) || !slotSave) {
              if (!slotConfirm) {
                slotConfirm = true;
                messageUntil = 0;
              } else if (state(c, slotPath, slotSave)) {
                notice(slotSave ? "State saved" : "State loaded");
                menuOpen = 0;
                companion = false;
                slotConfirm = false;
                slots_refresh(c, ss, slots);
                due = SDL_GetPerformanceCounter();
                if (audio)
                  SDL_ClearQueuedAudio(audio);
              } else {
                slotConfirm = false;
                notice("State operation failed; game retained");
                slots_refresh(c, ss, slots);
              }
            } else if (state(c, slotPath, true)) {
              notice("State saved");
              menuOpen = 0;
              companion = false;
              slots_refresh(c, ss, slots);
              due = SDL_GetPerformanceCounter();
            } else
              notice("State save failed; check storage");
          }
          held &= ~1023u;
        } else if (menuOpen == 6) {
          if (down(UP))
            pd_menu_move(&screenMenu, -1);
          if (down(DOWN))
            pd_menu_move(&screenMenu, 1);
          if (down(B))
            menuOpen = 1;
          else if (down(A) || down(RIGHT) || down(LEFT)) {
            int delta = down(LEFT) ? -1 : 1;
            switch (screenMenu.selected) {
            case 0: {
              int f = displayOptions.filter == 3 ? 2 : displayOptions.filter;
              f = (f + delta + 3) % 3;
              if (displayOptions.stretch && f)
                notice("Select ORIGINAL aspect before FILTER");
              else
                displayOptions.filter = f == 2 ? 3 : f;
              break;
            }
            case 1:
              displayOptions.grid = (displayOptions.grid + delta + 3) % 3;
              break;
            case 2:
              displayOptions.colors = !displayOptions.colors;
              break;
            case 3:
              if (displayOptions.filter && !displayOptions.stretch)
                notice("STRETCH with FILTER is not yet supported");
              else
                displayOptions.stretch = !displayOptions.stretch;
              break;
            case 4:
              wide_cycle();
              break;
            case 5:
              displayOptions.autoZoom = !displayOptions.autoZoom;
              break;
            case 6:
              displayOptions.autoResume = !displayOptions.autoResume;
              break;
            case 7:
              displayOptions.ffSpeed = 2 + (displayOptions.ffSpeed - 2 + delta + 3) % 3;
              settings.ffSpeed = displayOptions.ffSpeed;
              break;
            case 8:
              displayOptions.companionShaders = !displayOptions.companionShaders;
              break;
            case 9:
              displayOptions.battleBackground = !displayOptions.battleBackground;
              break;
            }
            effect_invalidate();
            options_save();
          }
        } else if (menuOpen == 4) {
          if (down(B) || down(A))
            menuOpen = 1;
        } else {
          struct pd_menu *pm = menuOpen == 1 ? &gameMenu : &confirmMenu;
          if (down(UP))
            pd_menu_move(pm, -1);
          if (down(DOWN))
            pd_menu_move(pm, 1);
          if (down(B)) {
            if (menuOpen == 1) {
              menuOpen = 0;
              companion = false;
            } else
              menuOpen = 1;
          } else if (down(A)) {
            if (menuOpen == 1) {
              switch (gameMenu.selected) {
              case 0:
                menuOpen = 0;
                companion = false;
                break;
              case 1:
                menuOpen = 0;
                ui.tab = PD_TAB_PARTY;
                pcUi.section = PC_PARTY;
                pcUi.summary = pcUi.detail = false;
                break;
              case 2:
                messageUntil = 0;
                menuOpen = 7;
                slotSave = true;
                slotConfirm = false;
                if (slots)
                  slots_refresh(c, ss, slots);
                break;
              case 3:
                messageUntil = 0;
                menuOpen = 7;
                slotSave = false;
                slotConfirm = false;
                if (slots)
                  slots_refresh(c, ss, slots);
                break;
              case 4:
                ff = !ff;
                notice(ff ? "Fast-forward on" : "Fast-forward off");
                break;
              case 5:
                if (displayOptions.autoResume) {
                  leave = true;
                  break;
                }
                menuOpen = 3;
                confirmMenu.title = displayOptions.autoResume
                                        ? "Close game? Auto resume ON"
                                        : "Close? Save in-game first";
                confirmMenu.selected = 0;
                break;
              case 6:
                menuOpen = 4;
                break;
              case 7:
                wide_cycle();
                break;
              case 9:
                menuOpen = 6;
                break;
              case 8:
                menuOpen = 1;
                quickOpen = true;
                quickSelected = 0;
                break;
              }
            } else {
              if (confirmMenu.selected == 1) {
                if (menuOpen == 3)
                  leave = true;
                else if (menuOpen == 5)
                  notice(state(c, ss, true) ? "State saved"
                                            : "State save failed");
                else
                  notice(state(c, ss, false) ? "State loaded"
                                             : "State load failed");
              }
              menuOpen = 1;
            }
          }
        }
      } else if (pc >= 0) {
        if (down(START) && pcUi.section == PC_BATTLE) {
          if (pcUi.battleSupported && snap.inBattle) {
            pcUi.battleControls = !pcUi.battleControls;
            pcUi.battleParty = false;
            pcUi.battleChoice = 0;
          } else
            notice("Battle controls need normal Polished Crystal 3.2.3 battle");
        }
        if (down(START) && pcUi.section == PC_BAG) {
          if (pocket_action_begin(&gameAction, displayKey, read_memory, c)) {
            companion = false;
            held &= ~1023u;
          }
          notice(pocket_action_message(&gameAction));
        }
        static const int buttons[] = {UP, DOWN, LEFT, RIGHT, A,
                                      B,  L,    R,    Y,     SELECT};
        for (unsigned action = 0; action < sizeof buttons / sizeof *buttons;
             action++)
          if (down(buttons[action]) &&
              !pc_ui_input(&pcUi, &snap, (enum pc_action)action))
            companion = false;
        if (favoriteRequested)
          pc_ui_input(&pcUi, &snap, PC_FAVORITE);
        if (pcUi.favoritesDirty) {
          if (!pocket_favorites_save(root, displayKey, &pcUi.favorites))
            notice("Favorites could not be saved");
          pcUi.favoritesDirty = false;
        }
        if (pcUi.battleRequest) {
          if (gameAction.result != PA_RUNNING &&
              pocket_battle_begin(
                  &battleAction, displayKey,
                  (enum pocket_battle_command)pcUi.battleRequest,
                  pcUi.battleValue, read_memory, c)) {
            companion = false;
            battleReturn = !(pcUi.battleRequest == PB_CHOOSE_ACTION &&
                             pcUi.battleValue == 1);
            held = pressed = 0;
          }
          pcUi.battleRequest = 0;
          notice(pocket_battle_message(&battleAction));
        }
      } else {
        if (down(B) && !pd_ui_back(&ui))
          companion = false;
        if (down(L) || down(R)) {
          pd_ui_next_tab(&ui, down(R) ? 1 : -1);
          focus = 0;
        }
        for (int k = RIGHT; k <= DOWN; k++)
          if (down(k))
            focus_move(&ui, &focus, k);
        if (down(A) && focus >= 0 && focus < ui.hitCount) {
          struct pd_hit hit = ui.hits[focus];
          pd_ui_touch(&ui, hit.x + hit.w / 2, hit.y + hit.h / 2, true);
          pd_ui_touch(&ui, hit.x + hit.w / 2, hit.y + hit.h / 2, false);
          focus = 0;
        }
        switch (pd_ui_take_action(&ui)) {
        case PD_ACTION_LEAVE_GAME:
          leave = true;
          break;
        case PD_ACTION_TOGGLE_FF:
          ff = !ff;
          break;
        case PD_ACTION_SAVE_STATE:
          notice(state(c, ss, true) ? "State saved" : "State save failed");
          break;
        case PD_ACTION_LOAD_STATE:
          notice(state(c, ss, false) ? "State loaded" : "State load failed");
          break;
        case PD_ACTION_SETTINGS_CHANGED:
          pd_settings_save(&settings, config);
          displayOptions.ffSpeed = settings.ffSpeed;
          options_save();
          break;
        default:
          break;
        }
      }
      if (down(L2)) {
        menuOpen = 5;
        confirmMenu.title = "Overwrite save state?";
        confirmMenu.selected = 0;
      }
      if (down(R2)) {
        menuOpen = 2;
        confirmMenu.title = "Load state? Current progress will rewind.";
        confirmMenu.selected = 0;
      }
    } else {
      muted = ff;
      bool battleDispatch = battleAction.result == PA_RUNNING;
      bool dispatching = gameAction.result == PA_RUNNING || battleDispatch;
      unsigned keys = battleDispatch
                          ? pocket_battle_tick(&battleAction, read_memory, c)
                      : gameAction.result == PA_RUNNING
                          ? pocket_action_tick(&gameAction, read_memory, c)
                          : held & 1023;
      c->setKeys(c, keys);
      if (battleDispatch && battleAction.result != PA_RUNNING) {
        held &= ~1023u;
        notice(pocket_battle_message(&battleAction));
        if (battleAction.result != PA_DONE) {
          companion = true;
          battleReturn = false;
        }
      }
      if (dispatching && !battleDispatch && gameAction.result != PA_RUNNING) {
        held &= ~1023u;
        notice(pocket_action_message(&gameAction));
        if (gameAction.result == PA_TIMEOUT || gameAction.result == PA_REJECTED)
          companion = true;
      }
      int count = dispatching ? 1 : ff ? settings.ffSpeed : 1;
      for (int i = 0; i < count; i++) {
        c->runFrame(c);
        wide_tick(c, i < count - 1);
        if (saveStatus.failed)
          break;
      }
      if (saveStatus.failed)
        break;
    }
    if (leave)
      break;
    audio_pause(companion || quickOpen);
    unsigned coreFrame = c->frameCounter(c);
    bool freshSnapshot = !cacheIdle || !snapshotValid || snapshotFrame != coreFrame ||
                         snapshotEpoch != observationEpoch;
    if (freshSnapshot) {
      snapshotReads++;
      if (pc >= 0) {
        pc_read(&snap, read_memory, c);
        pc_ui_observe(&pcUi, &snap);
      } else
        pd_snapshot_read(&snap, &game, read_memory, c);
      snapshotFrame = coreFrame;
      snapshotEpoch = observationEpoch;
      snapshotValid = true;
    }
    if (pc >= 0) {
      pc_ui_tick(&pcUi, SDL_GetTicks());
      if (pcUi.battleSupported && freshSnapshot) {
        if (!pocket_battle_read(read_memory, c, &pcUi.battleMenu))
          pcUi.battleMenu =
              (struct pocket_battle_menu){.state = PB_BUSY, .cursor = -1};
        if (pcUi.battleMenu.state == PB_NONE)
          battleReturn = false;
        if (battleReturn && battleAction.result != PA_RUNNING &&
            (pcUi.battleMenu.state == PB_ACTION ||
             pcUi.battleMenu.state == PB_MOVE ||
             pcUi.battleMenu.state == PB_PARTY)) {
          companion = true;
          battleReturn = false;
          pcUi.battleControls = true;
          pcUi.battleParty = false;
          pcUi.battleChoice =
              pcUi.battleMenu.cursor >= 0 && pcUi.battleMenu.cursor < 6
                  ? pcUi.battleMenu.cursor
                  : 0;
          held = pressed = 0;
          c->setKeys(c, 0);
          if (audio)
            SDL_ClearQueuedAudio(audio);
        }
      }
    }
    battleVisible = pc >= 0 && snap.valid && snap.inBattle;
    bool noticeVisible = SDL_GetTicks() < messageUntil;
    unsigned animation = companion && !quickOpen && !menuOpen && pc >= 0
                             ? pc_ui_animation_key(&pcUi) : 0;
    unsigned displayStamp = displayOptions.filter | (displayOptions.grid << 3) |
                            (displayOptions.colors << 5) |
                            (displayOptions.companionShaders << 6);
    bool redraw = !cacheIdle || !painted || (!companion && !quickOpen) || freshSnapshot ||
                  pressed || paintedEpoch != repaintEpoch ||
                  companion != paintedCompanion || quickOpen != paintedQuick ||
                  menuOpen != paintedMenu || animation != paintedAnimation ||
                  noticeVisible != paintedNotice ||
                  paintedMessageUntil != messageUntil ||
                  displayStamp != paintedDisplay || (companion && pc < 0);
    if (redraw && quickOpen) {
      quick_present(gw, gh, quickSelected, ff, access(ss, F_OK) == 0, romdata,
                    len, pc);
    } else if (redraw && companion) {
      if (menuOpen) {
        opts[5] =
            displayOptions.autoResume ? "Save session & close" : "Close game";
        if (menuOpen == 7) {
          slots_draw(&canvas, slots, slotIndex, slotSave, slotConfirm);
        } else if (menuOpen == 6) {
          static const char *filters[] = {"NONE", "LCD", "SCANLINES"};
          static const char *grids[] = {"SOFT", "MEDIUM", "STRONG"};
          snprintf(
              optionText[0], 80, "FILTER: %s",
              filters[displayOptions.filter == 3 ? 2 : displayOptions.filter]);
          snprintf(optionText[1], 80, "GRID: %s", grids[displayOptions.grid]);
          snprintf(optionText[2], 80, "GBA COLORS: %s",
                   displayOptions.colors ? "ON" : "OFF");
          snprintf(optionText[3], 80, "ASPECT: %s",
                   displayOptions.stretch ? "STRETCH" : "ORIGINAL");
          snprintf(optionText[4], 80, "EXPAND: %s",
                   wideMode == 1   ? "ON"
                   : wideMode == 2 ? "ZOOM OUT"
                                   : "OFF");
          snprintf(optionText[5], 80, "AUTO ZOOM IN: %s",
                   displayOptions.autoZoom ? "ON" : "OFF");
          snprintf(optionText[6], 80, "AUTO RESUME: %s",
                   displayOptions.autoResume ? "ON" : "OFF");
          snprintf(optionText[7], 80, "FAST-FORWARD: %dx", displayOptions.ffSpeed);
          snprintf(optionText[8], 80, "ON COMPANION: %s",
                   displayOptions.companionShaders ? "ON" : "OFF");
          snprintf(optionText[9], 80, "BATTLE BACKGROUND: %s",
                   displayOptions.battleBackground ? "ON" : "OFF");
          pd_menu_draw(&screenMenu, &canvas);
        } else if (menuOpen == 4) {
          pd_splash_draw(&canvas, "Based on PokeDaisy by Lidor Itzhari",
                         "https://github.com/lidor30/pokedaisy",
                         "GPL-3.0 | mGBA MPL-2.0 | B: Back");
        } else
          pd_menu_draw(menuOpen == 1 ? &gameMenu : &confirmMenu, &canvas);
      } else if (pc >= 0) {
        pc_ui_draw(&pcUi, &canvas, &snap, romdata, len, pc);
      } else {
        pd_ui_update(&ui, &snap);
        host.ffOn = ff;
        host.hasState = access(ss, F_OK) == 0;
        pd_ui_draw(&ui, &canvas, &game, &snap, &host);
        focus_draw(&ui, focus);
      }
      present_ui(!menuOpen);
    } else if (redraw) {
      present_game(gw, gh);
    }
    painted = true;
    paintedEpoch = repaintEpoch;
    paintedCompanion = companion;
    paintedQuick = quickOpen;
    paintedMenu = menuOpen;
    paintedAnimation = animation;
    paintedNotice = noticeVisible;
    paintedMessageUntil = messageUntil;
    paintedDisplay = displayStamp;
#ifdef POCKET_TEST
    testQuick = quickOpen;
    testFf = ff;
    testQuickSelected = quickSelected;
    testMenu = menuOpen;
    test_frame(c, &pc, companion, frames, &ui, &snap, &pcUi);
#endif
    frames++;
    if (maxFrames && frames >= maxFrames) {
      if (shot) {
        unsigned lit = 0;
        if (!capture_frame(shot, &lit))
          notice("Capture failed");
        fprintf(stderr,
                "Capture frame %u: %ux%u game, rect %d,%d %dx%d, lit=%u\n",
                c->frameCounter(c), gw, gh, lastGameRect.x, lastGameRect.y,
                lastGameRect.w, lastGameRect.h, lit);
      }
      break;
    }
    due += (uint64_t)(SDL_GetPerformanceFrequency() / hz);
    uint64_t now = SDL_GetPerformanceCounter();
    if (now < due)
      SDL_Delay((Uint32)((due - now) * 1000 / SDL_GetPerformanceFrequency()));
    else if (now - due > SDL_GetPerformanceFrequency() / 5)
      due = now;
  }
  if (audio)
    SDL_ClearQueuedAudio(audio);
  struct timespec cpuEnd = {0}, wallEnd = {0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpuEnd);
  clock_gettime(CLOCK_MONOTONIC, &wallEnd);
  fprintf(stderr, "Session work: wall=%.3fs process-cpu=%.3fs loops=%d draws=%llu snapshots=%llu\n",
          wallEnd.tv_sec - wallStart.tv_sec + (wallEnd.tv_nsec - wallStart.tv_nsec) / 1.e9,
          cpuEnd.tv_sec - cpuStart.tv_sec + (cpuEnd.tv_nsec - cpuStart.tv_nsec) / 1.e9,
          frames, (unsigned long long)framePresents, (unsigned long long)snapshotReads);
  bool recoveryOk = !displayOptions.autoResume || saveStatus.failed ||
                    pocket_recovery_save(c, root, displayKey, sav);
  if (!recoveryOk) {
    notice("Recovery save failed; check storage");
#ifndef POCKET_TEST
    while (running && !maxFrames && !recoveryOk) {
      pd_font_set(NULL, 0);
      pd_splash_draw(&canvas, "COULD NOT SAVE SESSION",
                     "Check SD card space and permissions.",
                     "A:Retry  B:Keep playing");
      present();
      events();
      if (down(A))
        recoveryOk = pocket_recovery_save(c, root, displayKey, sav);
      if (down(B)) {
        leave = false;
        companion = false;
        menuOpen = 0;
        quickOpen = false;
        held = pressed = 0;
        due = SDL_GetPerformanceCounter();
        goto play_loop;
      }
      SDL_Delay(16);
    }
#endif
    if (!recoveryOk)
      appError = 1;
  }
  free(slots);
cleanup:
  effect_invalidate();
  c->unloadROM(c);
  mCoreConfigDeinit(&c->config);
  c->deinit(c);
  if (sv && !saveStatus.closed)
    sv->close(sv); /* GBA leaves caller-owned save open. */
  free(romdata);
  held = pressed = 0;
  if (saveStatus.failed) {
    appError = 1;
    notice(backupMade ? "Save I/O failed. Pre-launch backup retained."
                      : "Save I/O failed. Check storage.");
#ifndef POCKET_TEST
    if (!maxFrames) {
      pd_font_set(NULL, 0);
      pd_splash_draw(&canvas, "COULD NOT SAVE",
                     backupMade ? "Check SD card. Pre-launch backup retained."
                                : "Check SD card space and permissions.",
                     "A/B: Return");
      present();
      while (running) {
        events();
        if (down(A) || down(B) || down(X) || down(MENU))
          break;
        SDL_Delay(16);
      }
    }
#endif
  }
}
static int cmp(const void *a, const void *b) {
  return strcasecmp(*(char *const *)a, *(char *const *)b);
}
int main(int argc, char **argv) {
  const char *rom = NULL, *shot = NULL;
  bool showLibrary = false;
  int frames = 0;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--data") && i + 1 < argc)
      root = argv[++i];
    else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
      frames = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--shot") && i + 1 < argc)
      shot = argv[++i];
    else if (!strcmp(argv[i], "--fresh"))
      forceFresh = true;
    else if (!strcmp(argv[i], "--library"))
      showLibrary = true;
    else
      rom = argv[i];
  }
  signal(SIGINT, stop);
  signal(SIGTERM, stop);
  mkdir(root, 0755);
  char path[1024];
  const char *dirs[] = {"roms", "saves", "states"};
  for (int i = 0; i < 3; i++) {
    snprintf(path, sizeof path, "%s/%s", root, dirs[i]);
    mkdir(path, 0755);
  }
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK |
               SDL_INIT_GAMECONTROLLER)) {
    fprintf(stderr, "SDL: %s\n", SDL_GetError());
    return 1;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  window = SDL_CreateWindow(
      "PokeDaisy Pocket", 0, 0, getenv("POCKET_FULLSCREEN") ? 1024 : W,
      getenv("POCKET_FULLSCREEN") ? 768 : H,
      getenv("POCKET_FULLSCREEN") ? SDL_WINDOW_FULLSCREEN : 0);
  renderer = window ? SDL_CreateRenderer(window, -1, 0) : NULL;
  if (!renderer) {
    fprintf(stderr, "Video: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_RendererInfo rendererInfo;
  if (!SDL_GetRendererInfo(renderer, &rendererInfo))
    fprintf(stderr, "Video backend: %s; opaque RGB888 canvas %dx%d\n",
            rendererInfo.name, W, H);
  SDL_RenderSetLogicalSize(renderer, W, H);
  SDL_ShowCursor(SDL_DISABLE);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                              SDL_TEXTUREACCESS_STREAMING, W, H);
  gameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING, 256, 256);
  wideTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                                  SDL_TEXTUREACCESS_STREAMING, 320, 240);
  if (!texture || !gameTexture || !wideTexture) {
    fprintf(stderr, "Texture: %s\n", SDL_GetError());
    return 1;
  }
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
  SDL_SetTextureBlendMode(gameTexture, SDL_BLENDMODE_NONE);
  SDL_SetTextureBlendMode(wideTexture, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  pd_canvas_init(&canvas, W, H, pixels);
  audio_open();
  pocket_platform_init(&platform);
  pocket_platform_log_energy(&platform, "start");
  SDL_GameController *pad = SDL_NumJoysticks() && SDL_IsGameController(0)
                                ? SDL_GameControllerOpen(0)
                                : NULL;
  SDL_Joystick *js = pad                  ? SDL_GameControllerGetJoystick(pad)
                     : SDL_NumJoysticks() ? SDL_JoystickOpen(0)
                                          : NULL;
  quick_bindings(js, pad);
  SDL_PumpEvents();
  SDL_JoystickUpdate();
  if ((js && SDL_JoystickGetButton(js, 0)) ||
      SDL_GetKeyboardState(NULL)[SDL_SCANCODE_X])
    forceFresh = true;
  if (rom)
    run_game(rom, frames, shot);
  else {
    char *names[256];
    int count = 0;
    snprintf(path, sizeof path, "%s/roms", root);
    DIR *d = opendir(path);
    struct dirent *e;
    while (d && (e = readdir(d)) && count < 256) {
      char *ext = strrchr(e->d_name, '.');
      if (ext && (!strcasecmp(ext, ".gba") || !strcasecmp(ext, ".gbc") ||
                  !strcasecmp(ext, ".gb")))
        names[count++] = strdup(e->d_name);
    }
    if (d)
      closedir(d);
    qsort(names, count, sizeof(*names), cmp);
    if (count == 1 && !showLibrary) {
      snprintf(path, sizeof path, "%s/roms/%s", root, names[0]);
      run_game(path, frames, shot);
      free(names[0]);
    } else {
      struct pd_menu menu;
      pd_menu_init(&menu, "PokeDaisy Pocket", (const char *const *)names,
                   count);
      menu.emptyHint = "Put .gb / .gbc / .gba files in roms/";
      menu.emptyPath = "";
      int ticks = 0;
      while (running) {
        events();
        if (lifecycle(NULL, NULL, NULL, NULL))
          continue;
        repeat_directions();
        if (down(B) || down(MENU))
          break;
        if (down(UP))
          pd_menu_move(&menu, -1);
        if (down(DOWN))
          pd_menu_move(&menu, 1);
        if (down(A) && count) {
          snprintf(path, sizeof path, "%s/roms/%s", root, names[menu.selected]);
          run_game(path, 0, NULL);
        }
        pd_menu_draw(&menu, &canvas);
        present();
        if (frames && ++ticks >= frames) {
          if (shot)
            capture_frame(shot, NULL);
          break;
        }
        SDL_Delay(16);
      }
      for (int i = 0; i < count; i++)
        free(names[i]);
    }
  }
  if (pad)
    SDL_GameControllerClose(pad);
  else if (js)
    SDL_JoystickClose(js);
  SDL_DestroyTexture(quickTexture);
  quickTexture = NULL;
  if (audio)
    SDL_CloseAudioDevice(audio);
  SDL_DestroyTexture(texture);
  SDL_DestroyTexture(gameTexture);
  SDL_DestroyTexture(wideTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  if (!pocket_platform_poweroff(&platform)) {
    fprintf(stderr, "NextUI shutdown request failed\n");
    appError = 1;
  }
  pocket_platform_log_energy(&platform, "exit");
  pocket_platform_close(&platform);
  return appError;
}
