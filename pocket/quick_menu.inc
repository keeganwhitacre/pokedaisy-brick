/* GPL-3.0. Controller adaptation of upstream QuickMenu: four offered host
 * actions. Game actions are omitted until their upstream implementations are
 * ported. */
#include "quick_icons.h"
#define QUICK_CLEAR 0x010203u
// SDL 2.26 Linux ConfigureJoystick orders BTN_JOYSTICK..KEY_MAX first,
// then lower key codes. Match that ordering for evdev-backed joysticks.
static int quick_evdev_button(const unsigned char *bits, int target,
                              int *total) {
  int index = 0, found = -1;
  for (int pass = 0; pass < 2; pass++)
    for (int code = pass ? 0 : BTN_JOYSTICK;
         code < (pass ? BTN_JOYSTICK : KEY_MAX); code++)
      if ((bits[code / 8] >> (code % 8)) & 1) {
        if (code == target)
          found = index;
        index++;
      }
  if (total)
    *total = index;
  return found;
}
static void quick_bindings(SDL_Joystick *js, SDL_GameController *pad) {
  l3Button = r3Button = -1;
  if (js) {
    // Optional since SDL 2.24: retain startup compatibility with older
    // firmware.
    typedef const char *(*path_fn)(SDL_Joystick *);
    path_fn joystick_path = (path_fn)dlsym(RTLD_DEFAULT, "SDL_JoystickPath");
    const char *path = joystick_path ? joystick_path(js) : NULL;
    int fd = path ? open(path, O_RDONLY | O_NONBLOCK) : -1;
    unsigned short buttons[KEY_MAX - BTN_MISC + 1] = {0};
    if (fd >= 0) {
      if (ioctl(fd, JSIOCGBTNMAP, buttons) >= 0)
        for (int i = 0; i < SDL_JoystickNumButtons(js) &&
                        i < (int)(sizeof buttons / sizeof *buttons);
             i++) {
          if (buttons[i] == BTN_THUMBL)
            l3Button = i;
          if (buttons[i] == BTN_THUMBR)
            r3Button = i;
        }
      // SDL normally opens /dev/input/eventN, where JSIOCGBTNMAP is
      // unavailable.
      unsigned char bits[(KEY_MAX + 7) / 8] = {0};
      if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof bits), bits) >= 0) {
        int count = 0;
        int left = quick_evdev_button(bits, BTN_THUMBL, &count);
        int right = quick_evdev_button(bits, BTN_THUMBR, NULL);
        if (count == SDL_JoystickNumButtons(js)) {
          l3Button = left;
          r3Button = right;
        }
      }
      close(fd);
    }
  }
  if (pad) {
    SDL_GameControllerButtonBind l = SDL_GameControllerGetBindForButton(
        pad, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    SDL_GameControllerButtonBind r = SDL_GameControllerGetBindForButton(
        pad, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    if (l.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON)
      l3Button = l.value.button;
    if (r.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON)
      r3Button = r.value.button;
  }
  const char *names[] = {"POCKET_L3_BUTTON", "POCKET_R3_BUTTON"};
  int *slots[] = {&l3Button, &r3Button};
  for (int i = 0; i < 2; i++) {
    const char *value = getenv(names[i]);
    if (value) {
      char *end;
      long v = strtol(value, &end, 10);
      if (*value && !*end && v >= 0 && v <= 255)
        *slots[i] = (int)v;
    }
  }
  if (l3Button >= 0 && l3Button == r3Button)
    r3Button = -1;
  fprintf(stderr,
          "Quick-menu bindings: L3 button %d, R3 button %d (zoom reserved); F3 "
          "keyboard/Menu > Quick menu fallback\n",
          l3Button, r3Button);
}
static void quick_draw(int selected, bool ff, bool hasState,
                       const unsigned char *rom, size_t n, int build) {
  pc_font(rom, n, build);
  pd_fill(&canvas, 0, 0, W, H, QUICK_CLEAR);
  const char *labels[] = {"SAVE STATE", "LOAD STATE", "FAST FWD", "COMPANION"};
  const int x[] = {172, 326, 172, 10}, y[] = {40, 150, 260, 150};
  for (int i = 0; i < 4; i++) {
    int w = i % 2 ? 176 : 168;
    pd_fill(&canvas, x[i], y[i], w, 80, i == selected ? 0x202838 : 0x808890);
    pd_fill(&canvas, x[i] + (i == selected ? 3 : 1),
            y[i] + (i == selected ? 3 : 1), w - (i == selected ? 6 : 2),
            80 - (i == selected ? 6 : 2),
            i == 1 && !hasState ? 0xb0b0b0 : 0xe8ecf4);
    int iw = quickArt[i].w, ih = quickArt[i].h;
    for (int yy = 0; yy < ih; yy++)
      for (int xx = 0; xx < iw; xx++) {
        unsigned char c = quickArt[i].rows[yy][xx];
        if (c != '.')
          pd_fill(&canvas, x[i] + (w - iw * 3) / 2 + xx * 3, y[i] + 6 + yy * 3,
                  3, 3, quickColors[c]);
      }
    pd_text_fit(&canvas, x[i] + 4, y[i] + 56, w - 8, labels[i], 0x202838,
                PD_NO_SHADOW);
  }
  // The centre and hint use an opaque backing to remain legible over any scene.
  pd_fill(&canvas, 192, 168, 128, 64, 0xe8ecf4);
  pd_text(&canvas, 216, 174, "QUICK", 0x202838, PD_NO_SHADOW);
  pd_text(&canvas, 224, 196, "MENU", 0x202838, PD_NO_SHADOW);
  const char *hints[] = {
      "Save current state", hasState ? "Load saved state" : "No save state yet",
      ff ? "Fast-forward: ON" : "Fast-forward: OFF", "Open companion"};
  pd_fill(&canvas, 4, 344, 504, 40, 0xe8ecf4);
  pd_text_fit(&canvas, 12, 346, 488, hints[selected], 0x202838, PD_NO_SHADOW);
  pd_text_fit(&canvas, 12, 366, 488, "D-pad:Choose A:Run B/L3:Back", 0x202838,
              PD_NO_SHADOW);
  pd_font_set(NULL, 0);
}
static void quick_present(unsigned gw, unsigned gh, int selected, bool ff,
                          bool hasState, const unsigned char *rom, size_t n,
                          int build) {
  render_game(gw, gh);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
  SDL_RenderFillRect(renderer, NULL);
  quick_draw(selected, ff, hasState, rom, n, build);
  static uint32_t overlay[W * H];
  for (int i = 0; i < W * H; i++)
    overlay[i] = pixels[i] == QUICK_CLEAR ? 0 : 0xff000000u | pixels[i];
  if (!quickTexture) {
    quickTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, W, H);
    if (quickTexture)
      SDL_SetTextureBlendMode(quickTexture, SDL_BLENDMODE_BLEND);
  }
  SDL_RenderSetLogicalSize(renderer, W, H);
  if (quickTexture && !SDL_UpdateTexture(quickTexture, NULL, overlay, W * 4))
    SDL_RenderCopy(renderer, quickTexture, NULL, NULL);
  else {
    notice("Quick menu rendering failed");
    appError = 1;
  }
  SDL_RenderPresent(renderer);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}
#undef QUICK_CLEAR
