/* GPL-3.0. Portable adaptations of upstream PartyScreen.drawHpBar,
 * BattleLists.drawListHpBar and MapScreen.drawMapCursor. No game memory or SDL.
 * Coordinates/width are logical pixels; the palette is supplied by each game.
 */
#ifndef POCKET_PIXEL_WIDGETS_H
#define POCKET_PIXEL_WIDGETS_H
#include "party_art.h"
#include "pd_canvas.h"
static inline void pocket_fade_rect(struct pd_canvas *c, int x, int y, int w,
                                    int h, uint32_t paper, int percent) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;
  int x0 = x > c->cx0 ? x : c->cx0, y0 = y > c->cy0 ? y : c->cy0;
  int x1 = x + w < c->cx1 ? x + w : c->cx1;
  int y1 = y + h < c->cy1 ? y + h : c->cy1;
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++) {
      uint32_t color = c->px[yy * c->w + xx], faded = 0;
      for (int shift = 0; shift < 24; shift += 8)
        faded |= ((((color >> shift) & 255) * percent +
                   ((paper >> shift) & 255) * (100 - percent)) /
                  100)
                 << shift;
      c->px[yy * c->w + xx] = faded;
    }
}
static inline int pocket_text_lines(struct pd_canvas *c, int width,
                                    const char *text) {
  if (width <= 0)
    return 0;
  struct pd_canvas measure = *c;
  measure.cx1 = measure.cx0;
  return pd_text_wrap(&measure, 0, 0, width, 18, 128, text, 0, PD_NO_SHADOW);
}
/* Scroll full wrapped text without truncating the last visible line. Reuse the
 * canvas wrapper's exact word/font metrics for measurement and drawing. */
static inline int pocket_text_page(struct pd_canvas *c, int x, int y, int width,
                                   int lineHeight, int visibleLines,
                                   const char *text, int *page, uint32_t ink,
                                   uint32_t shadow) {
  if (width <= 0 || lineHeight <= 0 || visibleLines <= 0)
    return 0;
  int lines = pocket_text_lines(c, width, text);
  int pages = lines ? (lines + visibleLines - 1) / visibleLines : 1;
  *page = *page < 0 ? 0 : *page % pages;
  struct pd_canvas clipped = *c;
  pd_clip(&clipped, x, y, width, visibleLines * lineHeight);
  if (clipped.cx0 < c->cx0)
    clipped.cx0 = c->cx0;
  if (clipped.cy0 < c->cy0)
    clipped.cy0 = c->cy0;
  if (clipped.cx1 > c->cx1)
    clipped.cx1 = c->cx1;
  if (clipped.cy1 > c->cy1)
    clipped.cy1 = c->cy1;
  pd_text_wrap(&clipped, x, y - *page * visibleLines * lineHeight, width,
               lineHeight, 128, text, ink, shadow);
  return pages;
}
struct pocket_hp_palette {
  uint32_t frame, inner, label, green, yellow, red;
};
static inline int pocket_hp_fill(int hp, int maxHp, int width) {
  if (maxHp <= 0 || hp <= 0 || width < 2)
    return 0;
  if (hp > maxHp)
    hp = maxHp;
  int filled = (int)((int64_t)(width / 2) * hp / maxHp) * 2;
  return filled < 2 ? 2 : filled;
}
/* x is the bar's left edge; the original HP label extends 22 pixels left. */
static inline void pocket_hp_bar(struct pd_canvas *c, int x, int y, int width,
                                 int hp, int maxHp,
                                 const struct pocket_hp_palette *p) {
  if (width < 10)
    return;
  pd_round_rect(c, x - 22, y, 24, 14, 2, p->frame);
  for (int row = 0; row < 5; row++)
    for (int col = 0; col < 9; col++)
      if (partyHpLetters[row][col] == '1')
        pd_fill(c, x - 20 + col * 2, y + 2 + row * 2, 2, 2, p->label);
  pd_round_rect(c, x, y, width, 14, 4, p->frame);
  pd_round_rect(c, x + 2, y + 2, width - 4, 10, 2, p->inner);
  uint32_t color = maxHp <= 0 || (int64_t)hp * 2 > maxHp ? p->green
                   : (int64_t)hp * 5 > maxHp             ? p->yellow
                                                         : p->red;
  pd_fill(c, x + 4, y + 4, pocket_hp_fill(hp, maxHp, width - 8), 6, color);
}
/* Original four 5x2 / 2x5 corners, 2 or 3 map pixels outside the region. */
static inline void pocket_map_cursor(struct pd_canvas *c, int left, int top,
                                     int right, int bottom, int scale, bool big,
                                     uint32_t color) {
  if (scale <= 0 || right <= left || bottom <= top)
    return;
  int out = (big ? 3 : 2) * scale;
  int x0 = left - out, y0 = top - out;
  int x1 = right + out, y1 = bottom + out;
  for (int i = 0; i < 4; i++) {
    int x = i & 1 ? x1 - 5 * scale : x0;
    int y = i & 2 ? y1 - 5 * scale : y0;
    pd_fill(c, x, i & 2 ? y1 - 2 * scale : y0, 5 * scale, 2 * scale, color);
    pd_fill(c, i & 1 ? x1 - 2 * scale : x0, y, 2 * scale, 5 * scale, color);
  }
}
#endif
