/* GPL-3.0. Controller adaptation of Android StatesScreen/SlotCard.
 * Same ten slots and OPTION windows; two columns fit the Brick's buttons. */
#include "pd_style.h"
static void slots_refresh(struct mCore *c, const char *quick,
                          struct pocket_slot *slots) {
  for (int i = 0; i < 10; i++) {
    char path[1200];
    pocket_slot_path(quick, i, path);
    pocket_slot_read(c, path, displayKey, slots + i);
  }
  // No filesystem polling while browsing. The host refreshes on entry/write.
}
static void slots_draw(struct pd_canvas *c, const struct pocket_slot *slots,
                       int selected, bool save, bool confirm) {
  pd_backdrop(c);
  pd_title_box(c, 4, 4, 504, 28, PD_TITLE_FILL);
  pd_text(c, 16, 10, save ? "SAVE STATES / SAVE" : "SAVE STATES / LOAD",
          PD_TITLE_TEXT, PD_TITLE_SHADOW);
  int top = selected / 2;
  if (top > 3)
    top = 3;
  for (int cell = 0; cell < 4; cell++) {
    int i = top * 2 + cell, x = 4 + (cell % 2) * 254, y = 40 + (cell / 2) * 144;
    pd_list_box(c, x, y, 250, 138);
    if (i == selected)
      pd_fill(c, x + 8, y + 8, 234, 122, PD_ROW_SELECTED);
    const struct pocket_slot *s = slots ? slots + i : NULL;
    char label[48];
    snprintf(label, sizeof label, "SLOT %d", i);
    pd_text(c, x + 12, y + 10, label, i == selected ? PD_VALUE : PD_LABEL,
            PD_NO_SHADOW);
    pd_fill(c, x + 12, y + 32, 112, 96, 0x20242c);
    if (s && s->width && s->height) {
      int w = 112, h = w * s->height / s->width;
      if (h > 96) {
        h = 96;
        w = h * s->width / s->height;
      }
      for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++)
          pd_fill(
              c, x + 12 + (112 - w) / 2 + xx, y + 32 + (96 - h) / 2 + yy, 1, 1,
              s->preview[(yy * s->height / h) * s->width + xx * s->width / w]);
    }
    const char *status = !s                          ? "Unavailable"
                         : s->status == SLOT_EMPTY   ? "Empty"
                         : s->status == SLOT_INVALID ? "Damaged"
                         : s->status == SLOT_LEGACY  ? "Old state"
                                                     : "Saved";
    pd_text_fit(c, x + 134, y + 38, 100, status, PD_LABEL, PD_NO_SHADOW);
    if (s && s->savedAt) {
      time_t t = s->savedAt;
      struct tm tm;
      if (localtime_r(&t, &tm)) {
        strftime(label, sizeof label, "%m/%d/%Y", &tm);
        pd_text_fit(c, x + 134, y + 66, 100, label, PD_LABEL, PD_NO_SHADOW);
        strftime(label, sizeof label, "%H:%M", &tm);
        pd_text(c, x + 134, y + 88, label, PD_LABEL, PD_NO_SHADOW);
      }
    }
  }
  pd_text_fit(c, 10, 336, 492,
              confirm ? (save ? "Overwrite this slot? A:Confirm B:Cancel"
                              : "Rewind to this state? A:Confirm B:Cancel")
                      : (save ? "D-pad:Slot A:Save Y:Load B:Game"
                              : "D-pad:Slot A:Load Y:Save B:Game"),
              PD_ON_BACKDROP, PD_ON_BACKDROP_SHADOW);
  pd_text(c, 10, 362, "Auto resume is separate", PD_ON_BACKDROP,
          PD_ON_BACKDROP_SHADOW);
}
