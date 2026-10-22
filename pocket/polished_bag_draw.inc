/* GPL-3.0. Upstream ItemsScreen's folder/list/description arrangement at the
 * Brick's logical size. Palette and source rules remain game-specific. */
static void pc_bag_pages(struct pc_ui *u, struct pd_canvas *c,
                         const struct pd_snapshot *s, const unsigned char *rom,
                         size_t n, int build) {
  bag_init(u, s);
  header(c, PC_BAG);
  // ItemsBackdrop: 2u stripes, using PolishedCrystalBag's exact two colors.
  for (int y = 34; y < 338; y += 4)
    pd_fill(c, 0, y, 512, 4, pcBagBackdrop[((y - 34) / 4) % 2]);
  struct pocket_item_ref rows[640];
  int count = bag_entries(s, u->pocket, u->bagSort, build, rows);
  if (u->bagReselect) {
    for (int i = 0; i < count; i++)
      if (rows[i].pocket == u->bagKeyPocket && rows[i].slot == u->bagKeySlot) {
        u->item = i;
        break;
      }
    u->bagReselect = false;
  }
  u->item = clamp_selection(u->item, count);
  if (count) {
    u->bagKeyPocket = rows[u->item].pocket;
    u->bagKeySlot = rows[u->item].slot;
  } else
    u->detail = false;
  char text[1024] = {0}, b[96];
  int lines = 1;
  if (u->detail && count) {
    item_description(rows[u->item].id, rom, n, build, text, sizeof text);
    lines = pocket_text_lines(c, 472, text);
    if (lines < 1)
      lines = 1;
    if (lines > 4)
      lines = 4;
  }
  int descTop = 334 - (lines * 20 + 28);
  int bottom = u->detail ? descTop - 10 : 334;
  pd_layered_box(c, 184, 38, 320, bottom - 38, 8, pcBagPanel, 4, 0xffffff);
  int tabs[6], tabCount = bag_tabs(s, tabs);
  for (int i = 0; i < tabCount; i++) {
    int y = 38 + i * 28;
    bool active = tabs[i] == u->pocket;
    pd_layered_box(c, 8, y, active ? 184 : 178, 28, 8, pcBagEdge, 3, 0xffffff);
    // Open the tab's right edge into the list's white inner band.
    pd_fill(c, 180, y + 6, active ? 12 : 4, 16, 0xffffff);
    if (active)
      pd_round_rect(c, 20, y + 19, 150, 4, 2, pcBagUnderline);
    txt(c, 20, y + 6, bag_labels[tabs[i] + 1]);
  }
  pd_layered_box(c, 8, 242, 176, 44, 8, pcBagEdge, 3, 0xffffff);
  txt(c, 20, 248, "Money");
  snprintf(b, sizeof b, "%ld", s->money);
  pd_text_right(c, 170, 264, b, 0x181818, PD_NO_SHADOW);
  const char *sorts[] = {"Default", "Name", "Type", "Count"};
  pd_layered_box(c, 8, 294, 176, 40, 8, pcBagEdge, 3, 0xffffff);
  snprintf(b, sizeof b, "Y:%s", sorts[u->bagSort]);
  txt(c, 20, 306, b);
  int visible = (bottom - 66) / 28;
  if (u->bagScroll > count - visible)
    u->bagScroll = count - visible;
  if (u->bagScroll < 0)
    u->bagScroll = 0;
  if (u->item < u->bagScroll)
    u->bagScroll = u->item;
  if (u->item >= u->bagScroll + visible)
    u->bagScroll = u->item - visible + 1;
  if (!count)
    pd_text_fit(c, 206, 54, 282, s->bagOk ? "No items" : "Bag not ready",
                0x181818, PD_NO_SHADOW);
  for (int row = 0; row < visible && u->bagScroll + row < count; row++) {
    int i = u->bagScroll + row, y = 50 + row * 28;
    if (i == u->item)
      for (int dx = 0; dx < 8; dx += 2)
        pd_fill(c, 202 + dx, y + dx / 2 + 2, 2, 14 - dx, 0x181818);
    pd_text_fit(c, 220, y + 2, 194, rows[i].name, 0x181818, PD_NO_SHADOW);
    snprintf(b, sizeof b, "x%d", rows[i].quantity);
    pd_text_right(c, 490, y + 2, b, 0x181818, PD_NO_SHADOW);
  }
  snprintf(b, sizeof b, "%d/%d", count ? u->item + 1 : 0, count);
  pd_text_right(c, 490, bottom - 24, b, 0x181818, PD_NO_SHADOW);
  if (u->detail && count) {
    pd_layered_box(c, 8, descTop, 496, 334 - descTop, 8, pcBagEdge, 3,
                   0xffffff);
    int pages = pocket_text_page(c, 20, descTop + 14, 472, 20, lines, text,
                                 &u->bagDescPage, 0x181818, PD_NO_SHADOW);
    snprintf(b, sizeof b, "Y:Sort A:Close Select:%d/%d", u->bagDescPage + 1,
             pages);
  } else
    snprintf(b, sizeof b, "D-pad:Browse A:Info Y:Sort");
  pd_text_fit(c, 12, 346, 488, b, 0x181818, PD_NO_SHADOW);
  txt(c, 12, 366, "L/R:Section B:Back X:Play");
}
