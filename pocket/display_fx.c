#include "display_fx.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static bool option_path(const char *root, const char *sha, char path[1200]) {
  if (!sha || strlen(sha) != 40)
    return false;
  for (int i = 0; i < 40; i++)
    if (!((sha[i] >= '0' && sha[i] <= '9') || (sha[i] >= 'a' && sha[i] <= 'f')))
      return false;
  return snprintf(path, 1200, "%s/display/%s.fx", root, sha) < 1200;
}
static bool valid(const struct pocket_display_options *o) {
  return (o->filter == 0 || o->filter == 1 || o->filter == 3) && o->grid >= 0 &&
         o->grid < 3 && !(o->stretch && o->filter);
}
bool pocket_options_load(const char *root, const char *sha,
                         struct pocket_display_options *out) {
  *out = (struct pocket_display_options){.grid = 1};
  char path[1200];
  if (!option_path(root, sha, path))
    return false;
  FILE *f = fopen(path, "r");
  if (!f)
    return false;
  char data[256];
  size_t n = fread(data, 1, sizeof data - 1, f);
  bool ok = !ferror(f) && n < sizeof data - 1 && !memchr(data, 0, n);
  if (fclose(f))
    ok = false;
  data[n] = 0;
  static const char *keys[] = {"filter=", "grid=", "colors=", "stretch="};
  int values[4] = {0};
  char *p = data;
  if (strncmp(p, "pocket-fx=1\n", 12))
    ok = false;
  else
    p += 12;
  for (int i = 0; ok && i < 4; i++) {
    size_t k = strlen(keys[i]);
    if (strncmp(p, keys[i], k)) {
      ok = false;
      break;
    }
    p += k;
    char *end;
    errno = 0;
    long v = strtol(p, &end, 10);
    if (errno || end == p || *end != '\n' || v < 0 || v > 3) {
      ok = false;
      break;
    }
    values[i] = (int)v;
    p = end + 1;
  }
  struct pocket_display_options o = {values[0], values[1], values[2] == 1,
                                     values[3] == 1};
  ok = ok && !*p && values[2] <= 1 && values[3] <= 1 && valid(&o);
  if (ok)
    *out = o;
  return ok;
}
bool pocket_options_save(const char *root, const char *sha,
                         const struct pocket_display_options *o) {
  char path[1200], temp[1220], dir[1200];
  if (!valid(o) || !option_path(root, sha, path))
    return false;
  if (snprintf(dir, sizeof dir, "%s/display", root) >= (int)sizeof dir)
    return false;
  mkdir(dir, 0755);
  snprintf(temp, sizeof temp, "%s.tmp", path);
  FILE *f = fopen(temp, "w");
  if (!f)
    return false;
  bool ok =
      fprintf(f, "pocket-fx=1\nfilter=%d\ngrid=%d\ncolors=%d\nstretch=%d\n",
              o->filter, o->grid, o->colors, o->stretch) > 0;
  if (fflush(f) || fsync(fileno(f)))
    ok = false;
  if (fclose(f))
    ok = false;
  if (ok)
    ok = rename(temp, path) == 0;
  if (!ok)
    unlink(temp);
  return ok;
}
uint32_t pocket_filter_mask(int filter, int grid, int x, int y, int w, int h,
                            int tw, int th) {
  if (w <= 0 || h <= 0 || tw <= 0 || th <= 0 || x < 0 || y < 0 || x >= w ||
      y >= h)
    return 0;
  // Pixel-centre evaluation of ScreenShaders LCD / SCANLINES. Integer geometry
  // avoids floating-point boundary drift at large expanded-frame dimensions.
  int alpha = 0;
  if (filter == POCKET_FILTER_LCD && grid >= 0 && grid < 3) {
    bool line = ((int64_t)(2 * x + 1) * tw * 3 / (2 * w)) % 3 == 0 ||
                ((int64_t)(2 * y + 1) * th * 3 / (2 * h)) % 3 == 0;
    static const int strengths[] = {51, 89, 128};
    if (line)
      alpha = strengths[grid];
  } else if (filter == POCKET_FILTER_SCANLINES) {
    if (((int64_t)(2 * y + 1) * th / h) % 2 == 0)
      alpha = 128;
  }
  return (uint32_t)alpha << 24; // black, with straight alpha for SDL blend
}
void pocket_color_lut(uint32_t table[32768]) {
  // ScreenShaders.GBA_COLOR: GLSL's mat3 is column-major. Preserve defaults.
  for (int b = 0; b < 32; b++)
    for (int g = 0; g < 32; g++)
      for (int r = 0; r < 32; r++) {
        double in[3] = {pow(((r << 3) | (r >> 2)) / 255., 2.7) * .99,
                        pow(((g << 3) | (g >> 2)) / 255., 2.7) * .99,
                        pow(((b << 3) | (b >> 2)) / 255., 2.7) * .99};
        double out[3] = {.84 * in[0] + .18 * in[1],
                         .09 * in[0] + .67 * in[1] + .26 * in[2],
                         .15 * in[0] + .10 * in[1] + .73 * in[2]};
        uint32_t rgb = 0;
        for (int i = 0; i < 3; i++) {
          if (out[i] > 1)
            out[i] = 1;
          int v = (int)lround(pow(out[i], .4625) * 255);
          rgb = (rgb << 8) | v;
        }
        table[r | (g << 5) | (b << 10)] = rgb;
      }
}
