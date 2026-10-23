/* GPL-3.0. Portable subset of upstream ScreenShaders/CompanionSettings.
 * LCD and SCANLINES retain original texel-space equations and GRID strengths.
 * No emulator, game ids, or GPU dependencies. Shader attribution: ../NOTICE. */
#ifndef POCKET_DISPLAY_FX_H
#define POCKET_DISPLAY_FX_H
#include <stdbool.h>
#include <stdint.h>
struct pocket_display_options {
  int filter, grid;
  bool colors, stretch;
  bool autoZoom, autoResume;
};
enum {
  POCKET_FILTER_NONE = 0,
  POCKET_FILTER_LCD = 1,
  POCKET_FILTER_SCANLINES = 3
};
bool pocket_options_load(const char *root, const char *sha,
                         struct pocket_display_options *);
bool pocket_options_save(const char *root, const char *sha,
                         const struct pocket_display_options *);
uint32_t pocket_filter_mask(int filter, int grid, int x, int y, int w, int h,
                            int texw, int texh);
void pocket_color_lut(uint32_t table[32768]);
static inline uint32_t pocket_color_apply(const uint32_t table[32768],
                                          uint32_t rgb) {
  return table[((rgb >> 19) & 31) | (((rgb >> 11) & 31) << 5) |
               (((rgb >> 3) & 31) << 10)];
}
#endif
