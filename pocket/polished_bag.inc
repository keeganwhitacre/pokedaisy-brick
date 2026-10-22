/* GPL-3.0. Controller adaptation of ItemsScreen's PolishedCrystalBag,
 * FolderTab, ItemRow and ItemDescriptionBar. No writes to game memory. */
static const char *bag_labels[] = {"All",       "Items",   "Poke Balls",
                                   "TMs & HMs", "Berries", "Key Items"};
static int bag_count(const struct pd_snapshot *s, int category) {
  int count = 0;
  if (!s->bagOk)
    return 0;
  for (int p = 0; p < PD_POCKET_COUNT; p++)
    if (category == -1 || category == p) {
      int n = s->bag[p].count;
      if (n > 0 && n <= PD_POCKET_SLOTS)
        count += n;
    }
  return count;
}
static int bag_tabs(const struct pd_snapshot *s, int tabs[6]) {
  int count = 1;
  tabs[0] = -1;
  for (int p = 0; p < PD_POCKET_COUNT; p++)
    if (bag_count(s, p))
      tabs[count++] = p;
  // Upstream omits the redundant category when ALL is the same single pocket.
  return count == 2 ? 1 : count;
}
static void bag_init(struct pc_ui *u, const struct pd_snapshot *s) {
  if (!u->bagStarted) {
    u->bagStarted = true;
    u->pocket = -1;
    u->detail = bag_count(s, -1) > 0;
  }
  int tabs[6], count = bag_tabs(s, tabs);
  bool found = false;
  for (int i = 0; i < count; i++)
    found |= tabs[i] == u->pocket;
  if (!found) {
    u->pocket = -1;
    u->item = u->bagScroll = u->bagDescPage = 0;
    u->bagReselect = false;
  }
}
static int bag_entries(const struct pd_snapshot *s, int category, int sort,
                       int build, struct pocket_item_ref rows[640]) {
  int count = 0;
  for (int p = 0; p < PD_POCKET_COUNT; p++)
    if (s->bagOk && (category == -1 || category == p) && s->bag[p].count >= 0 &&
        s->bag[p].count <= PD_POCKET_SLOTS)
      for (int i = 0; i < s->bag[p].count; i++) {
        const struct pd_item *item = &s->bag[p].items[i];
        rows[count++] = (struct pocket_item_ref){
            item_name(item->id, build), item->id, item->quantity, p, i};
      }
  struct pocket_item_ref scratch[640];
  pocket_item_sort(rows, scratch, count, sort);
  return count;
}
static void bag_input(struct pc_ui *u, const struct pd_snapshot *s,
                      enum pc_action a) {
  bag_init(u, s);
  if (a == PC_LEFT || a == PC_RIGHT) {
    int tabs[6], count = bag_tabs(s, tabs), selected = 0;
    for (int i = 0; i < count; i++)
      if (tabs[i] == u->pocket)
        selected = i;
    u->pocket = tabs[(selected + count + (a == PC_RIGHT ? 1 : -1)) % count];
    u->item = u->bagScroll = u->bagDescPage = 0;
    u->detail = u->bagReselect = false;
  }
  int count = bag_count(s, u->pocket);
  if (a == PC_UP || a == PC_DOWN) {
    u->item += a == PC_DOWN ? 1 : -1;
    u->detail = count > 0;
    u->bagDescPage = 0;
    u->bagReselect = false;
  }
  if (a == PC_CONFIRM) {
    u->detail = count > 0 && !u->detail;
    u->bagDescPage = 0;
  }
  if (a == PC_ALTERNATE) {
    u->bagSort = (u->bagSort + 1) % 4;
    u->bagReselect = count > 0;
    u->bagScroll = u->bagDescPage = 0;
  }
  if (a == PC_PLACES && u->detail)
    u->bagDescPage = (u->bagDescPage + 1) % 1024;
  if (u->item < 0 || !count)
    u->item = 0;
  if (count && u->item >= count)
    u->item = count - 1;
}
static void pc_bag_pages(struct pc_ui *u, struct pd_canvas *c,
                         const struct pd_snapshot *s, const unsigned char *rom,
                         size_t n, int build);
