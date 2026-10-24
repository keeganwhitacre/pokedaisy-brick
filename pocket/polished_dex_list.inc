/* GPL-3.0. PokedexScreen: counts/filter column + icon/number/name list.
 * The Brick stacks number/name within a row to retain readable ROM glyphs. */
static void dex_mark(struct pd_canvas *c, int x, int y, bool caught,
                     bool seen) {
  if (!seen && !caught)
    return;
  const uint32_t *colors = caught ? dexCaughtBall : dexSeenBall;
  for (int yy = 0; yy < 12; yy++)
    for (int xx = 0; xx < 12; xx++) {
      unsigned char ch = dexBall[yy][xx];
      if (ch != '.')
        pd_fill(c, x + xx, y + yy, 1, 1, colors[ch]);
    }
}
static void pc_dex_list(struct pd_canvas *c, const struct pd_snapshot *s,
                        int selected, const unsigned char *rom, size_t n,
                        int build, int filter, int *scroll) {
  int ids[512], count = dex_entries(s, filter, ids);
  int pos = dex_position(selected, ids, count), seen = 0, caught = 0;
  for (int i = 1; i <= pc_dex_count(); i++) {
    seen += pd_dex_seen(s, i);
    caught += pd_dex_caught(s, i);
  }
  pd_layered_box(c, 8, 38, 176, 44, 4, pcOptionFrame, 5, 0xffffff);
  txt(c, 22, 52, "POKéDEX");
  pd_layered_box(c, 8, 86, 176, 72, 4, pcOptionFrame, 5, 0xffffff);
  txt(c, 22, 100, "SEEN");
  txt(c, 22, 124, "OWNED");
  char b[128];
  snprintf(b, sizeof b, "%d", seen);
  pd_text_right(c, 168, 100, b, 0x181818, PD_NO_SHADOW);
  snprintf(b, sizeof b, "%d", caught);
  pd_text_right(c, 168, 124, b, 0x181818, PD_NO_SHADOW);
  pd_layered_box(c, 8, 164, 176, 170, 4, pcOptionFrame, 5, 0xffffff);
  const char *labels[] = {"ALL", "CAUGHT", "SEEN ONLY", "NOT SEEN"};
  const int counts[] = {pc_dex_count(), caught, seen - caught,
                        pc_dex_count() - seen};
  for (int f = 0; f < 4; f++) {
    int y = 176 + f * 36;
    if (filter == f)
      pd_fill(c, 20, y, 152, 34, 0xdcdcdc);
    txt(c, 22, y + 2, labels[f]);
    snprintf(b, sizeof b, "%d", counts[f]);
    pd_text_right(c, 168, y + 18, b, 0x181818, PD_NO_SHADOW);
  }
  pd_layered_box(c, 192, 38, 312, 296, 4, pcOptionFrame, 5, 0xffffff);
  if (*scroll > count - 5)
    *scroll = count - 5;
  if (*scroll < 0)
    *scroll = 0;
  if (pos < *scroll)
    *scroll = pos;
  if (pos >= *scroll + 5)
    *scroll = pos - 4;
  if (!count)
    txt(c, 300, 174, "NONE");
  for (int row = 0; row < 5 && *scroll + row < count; row++) {
    int i = ids[*scroll + row], id = dexOrder[i], y = 50 + row * 48;
    bool known = pd_dex_seen(s, i + 1), owned = pd_dex_caught(s, i + 1);
    uint32_t paper = i == selected ? 0xdcdcdc : 0xffffff;
    pd_fill(c, 204, y, 288, 46, paper);
    dex_mark(c, 206, y + 18, owned, known);
    icon(c, 224, y + 8, id, rom, n, build);
    if (!known)
      pocket_fade_rect(c, 224, y + 8, 32, 32, paper, 45);
    snprintf(b, sizeof b, "No.%03d", i + 1);
    pd_text(c, 264, y + 4, b, 0x787878, PD_NO_SHADOW);
    // Android keeps unencountered species names/icons visible, but muted.
    pd_text_fit(c, 264, y + 26, 226, names[id], known ? 0x181818 : 0x787878,
                PD_NO_SHADOW);
    for (int x = 212; x < 484; x += 16)
      pd_fill(c, x, y + 46, 12, 2, 0xb0b0b0);
  }
  snprintf(b, sizeof b, "%d/%d", count ? pos + 1 : 0, count);
  pd_text_right(c, 488, 306, b, 0x181818, PD_NO_SHADOW);
  txt(c, 12, 346, "D-pad:Browse Y:Filter A:Entry");
  txt(c, 12, 366, "L/R:Section B/X:Play");
}
