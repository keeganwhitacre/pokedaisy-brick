/* GPL-3.0. Adapted from Gen2Art.townMap/player, MapScreen and
 * MonDetailScreen. ROM art is decoded only from the user's matching cart. */
static int landmark(unsigned map) {
  for (unsigned i = 0; i < sizeof mapLinks / sizeof *mapLinks; i++)
    if (mapLinks[i].map == map)
      return mapLinks[i].landmark;
  return 0;
}
static bool map_image(uint32_t *pixels, const unsigned char *rom, size_t n,
                      int build, int region) {
  if (!rom || !build_ok(build) || region < 0 || region > 2)
    return false;
  const unsigned *a = builds[build].at;
  unsigned char tiles[0x4000];
  int bytes = lz(rom, n, off(a[5]), tiles, sizeof tiles);
  size_t map = off(a[6 + region]), pals = off(a[10]), palMap = off(a[9]);
  if (bytes < 0 || map + 360 > n || pals + 64 > n || palMap + 32 > n)
    return false;
  for (int cell = 0; cell < 360; cell++) {
    unsigned attr = rom[map + cell];
    if (attr == 255)
      return false;
    int tile = attr & 63;
    if (tile * 16 + 16 > bytes)
      return false;
    unsigned palette = (rom[palMap + tile / 2] >> (tile % 2 ? 4 : 0)) & 7;
    for (int y = 0; y < 8; y++)
      for (int x = 0; x < 8; x++) {
        int xx = attr & 64 ? 7 - x : x, yy = attr & 128 ? 7 - y : y;
        unsigned v = ((tiles[tile * 16 + 2 * yy] >> (7 - xx)) & 1) |
                     (((tiles[tile * 16 + 2 * yy + 1] >> (7 - xx)) & 1) << 1);
        pixels[(cell / 20 * 8 + y) * 160 + cell % 20 * 8 + x] =
            rgb(rom + pals + 8 * palette + 2 * v);
      }
  }
  return true;
}
static void player_head(struct pd_canvas *c, int x, int y, int gender,
                        const unsigned char *rom, size_t n, int build) {
  if (!rom || !build_ok(build))
    return;
  if (gender < 0 || gender > 2)
    gender = 0;
  const unsigned *a = builds[build].at;
  unsigned char tiles[0x4000];
  if (lz(rom, n, off(a[17 + gender]), tiles, sizeof tiles) < 64)
    return;
  size_t pal = off(a[16]) + 8 * gender;
  if (pal + 8 > n)
    return;
  for (int yy = 0; yy < 16; yy++)
    for (int xx = 0; xx < 16; xx++) {
      int tile = yy / 8 * 2 + xx / 8, row = tile * 16 + yy % 8 * 2;
      unsigned v = ((tiles[row] >> (7 - xx % 8)) & 1) |
                   (((tiles[row + 1] >> (7 - xx % 8)) & 1) << 1);
      if (v)
        pd_fill(c, x + 2 * xx, y + 2 * yy, 2, 2, rgb(rom + pal + 2 * v));
    }
}
static void map_center(struct pc_ui *u, const struct pd_snapshot *s) {
  int p = s->mapsec;
  if (p > 0 && p < (int)(sizeof places / sizeof *places) &&
      places[p].region >= 0) {
    u->region = places[p].region;
    u->mapX = places[p].x;
    u->mapY = places[p].y;
  } else {
    u->region = 0;
    u->mapX = 10;
    u->mapY = 9;
  }
  u->mapSet = true;
}
static int cursor_place(const struct pc_ui *u) {
  for (unsigned i = 1; i < sizeof places / sizeof *places; i++)
    if (places[i].region == u->region && u->mapX >= places[i].x &&
        u->mapX < places[i].x + places[i].w && u->mapY >= places[i].y &&
        u->mapY < places[i].y + places[i].h)
      return i;
  return 0;
}
static void pc_map_draw(struct pc_ui *u, struct pd_canvas *c,
                        const struct pd_snapshot *s, const unsigned char *rom,
                        size_t n, int build) {
  header(c, PC_MAP);
  if (!u->mapSet)
    map_center(u, s);
  if (u->placesOpen) {
    txt(c, 12, 36, "PLACES");
    int scroll = u->place >= 11 ? u->place - 10 : 0;
    for (int row = 0;
         row < 11 && scroll + row < (int)(sizeof places / sizeof *places) - 1;
         row++) {
      int i = scroll + row + 1, y = 62 + 24 * row;
      if (i == u->place + 1)
        pd_fill(c, 8, y, 496, 23, 0xdcdcdc);
      pd_text_fit(c, 18, y + 2, 478, places[i].name, 0x181818, PD_NO_SHADOW);
    }
    txt(c, 12, 346, "D-pad:Choose A:Show");
    txt(c, 12, 366, "B:Map X:Play");
    return;
  }
  uint32_t map[160 * 144];
  if (!map_image(map, rom, n, build, u->region)) {
    txt(c, 12, 78, "Map needs matching ROM.");
    return;
  }
  const char *regions[] = {"JOHTO", "KANTO", "ORANGE"};
  char title[128];
  int p = cursor_place(u);
  snprintf(title, sizeof title, "%s: %s", regions[u->region],
           p ? places[p].name : "");
  pd_text_fit(c, 12, 34, 488, title, 0x181818, PD_NO_SHADOW);
  for (int y = 0; y < 144; y++)
    for (int x = 0; x < 160; x++)
      pd_fill(c, 96 + 2 * x, 52 + 2 * y, 2, 2, map[y * 160 + x]);
  if (s->posOk && s->mapsec > 0 &&
      s->mapsec < (int)(sizeof places / sizeof *places) &&
      places[s->mapsec].region == u->region)
    player_head(c, 96 + places[s->mapsec].x * 16 - 8,
                52 + places[s->mapsec].y * 16 - 8, s->gender, rom, n, build);
  int cx = 96 + u->mapX * 16, cy = 52 + u->mapY * 16;
  // MapScreen frames the selected landmark, or one cell in empty space.
  int w = p ? places[p].w : 1, h = p ? places[p].h : 1;
  if (p) {
    cx = 96 + places[p].x * 16;
    cy = 52 + places[p].y * 16;
  }
  pocket_map_cursor(c, cx, cy, cx + w * 16, cy + h * 16, 2,
                    (u->animationMs / 333) % 2, 0xffffff);
  txt(c, 12, 346, "A:Region Y:Me Select:Places");
  txt(c, 12, 366, "D-pad:Look B/X:Play");
}
static const int *mon_types(int species, int build) {
  if (species < 0 || species >= 512)
    return speciesTypes[0];
  return build_ok(build) && builds[build].faithful && faithfulTypes[species][0]
             ? faithfulTypes[species]
             : speciesTypes[species];
}
static int type_percent(int attack, int defense) {
  for (unsigned i = 0; i < sizeof typeMatchups / sizeof *typeMatchups; i++)
    if (typeMatchups[i].pair == attack * 100 + defense)
      return typeMatchups[i].percent;
  return 100;
}
static void pc_summary_draw(struct pd_canvas *c, const struct pd_snapshot *s,
                            int selected, int move, int page, int typePage,
                            const unsigned char *rom, size_t n, int build) {
  header(c, PC_PARTY);
  const struct pd_mon *m = &s->party[selected];
  char b[128];
  pd_fill(c, 8, 40, 496, 294, 0x181818);
  pd_fill(c, 10, 42, 492, 290, 0xffffff);
  pd_fill(c, 250, 42, 2, 290, 0x181818);
  pd_text_fit(c, 20, 52, 218, m->species == 255 ? "Egg" : names[m->species],
              0x181818, PD_NO_SHADOW);
  icon(c, 20, 80, m->species, rom, n, build);
  if (m->species == 255) {
    txt(c, 20, 138, "It hasn't hatched yet.");
    txt(c, 12, 346, "Up/Down:Mon B:Party");
    txt(c, 12, 366, "X:Play");
    return;
  }
  snprintf(b, sizeof b, "Lv%d %s", m->level,
           m->gender == PD_GENDER_FEMALE ? "♀"
           : m->gender == PD_GENDER_MALE ? "♂"
                                         : "");
  txt(c, 68, 88, b);
  snprintf(b, sizeof b, "HP %d/%d", m->hp, m->maxHp);
  txt(c, 20, 124, b);
  pd_fill(c, 20, 144, 218, 6, 0x181818);
  pd_fill(c, 22, 146, 214, 2, 0xffffff);
  if (m->maxHp > 0)
    pd_fill(c, 22, 146, 214 * m->hp / m->maxHp, 2,
            m->hp * 2 > m->maxHp   ? 0x00bd00
            : m->hp * 5 > m->maxHp ? 0xffbd00
                                   : 0xff0000);
  const int *types = mon_types(m->species, build);
  if (!page) {
    if (types[0]) {
      txt(c, 20, 154, typeNames[types[1]]);
      if (types[1] != types[2])
        txt(c, 20, 176, typeNames[types[2]]);
    }
    txt(c, 20, 208, "HELD ITEM");
    pd_text_fit(c, 20, 230, 218, item_name(m->heldItem, build), 0x181818,
                PD_NO_SHADOW);
    txt(c, 20, 250, "EXPERIENCE");
    snprintf(b, sizeof b, "%lu", (unsigned long)m->experience);
    txt(c, 20, 272, b);
    struct pocket_exp xp;
    if (pc_exp(m, 214, &xp)) {
      pd_fill(c, 20, 296, 218, 10, 0x181818);
      pd_fill(c, 22, 298, 214, 6, 0xffffff);
      pd_fill(c, 22, 298, xp.filled, 6, 0x40b8f8);
      if (m->level == 100)
        snprintf(b, sizeof b, "MAX");
      else
        snprintf(b, sizeof b, "NEXT %lu", (unsigned long)xp.toNext);
      txt(c, 20, 314, b);
    }
  } else {
    const char *labels[] = {"", "WEAK TO", "RESISTS", "IMMUNE TO"};
    txt(c, 20, 158, labels[page]);
    int ids[18], values[18], count = 0;
    if (types[0])
      for (int attack = 0; attack < 18; attack++) {
        int percent = type_percent(attack, types[1]);
        if (types[1] != types[2])
          percent = percent * type_percent(attack, types[2]) / 100;
        if ((page == 1 && percent > 100) ||
            (page == 2 && percent > 0 && percent < 100) ||
            (page == 3 && !percent)) {
          ids[count] = attack;
          values[count++] = percent;
        }
      }
    int start = count ? (typePage % ((count + 5) / 6)) * 6 : 0;
    for (int row = 0; row < 6 && start + row < count; row++) {
      int k = start + row, v = values[k];
      snprintf(b, sizeof b, "%s %s", typeNames[ids[k]],
               v == 400   ? "x4"
               : v == 200 ? "x2"
               : v == 50  ? "x1/2"
               : v == 25  ? "x1/4"
                          : "x0");
      pd_text_fit(c, 20, 182 + 22 * row, 218, b, 0x181818, PD_NO_SHADOW);
    }
    if (!count)
      txt(c, 20, 182, "None");
  }
  bool faithful = build_ok(build) && builds[build].faithful;
  for (int k = 0; k < 4; k++) {
    int y = 46 + k * 70, id = m->moves[k];
    if (k == move)
      pd_fill(c, 258, y, 238, 68, 0xdcdcdc);
    if (!id) {
      txt(c, 266, y + 5, "-");
      continue;
    }
    bool change = faithful && faithfulMoves[id].name;
    const char *name = change ? faithfulMoves[id].name : moves[id].name;
    int type = change ? faithfulMoves[id].type : moves[id].type,
        power = change ? faithfulMoves[id].power : moves[id].power;
    pd_text_fit(c, 266, y + 4, 226, name ? name : "Unknown", 0x181818,
                PD_NO_SHADOW);
    txt(c, 266, y + 25, typeNames[type] ? typeNames[type] : "?");
    snprintf(b, sizeof b, "%dPP", m->pp[k]);
    pd_text_right(c, 492, y + 25, b, 0x181818, PD_NO_SHADOW);
    snprintf(b, sizeof b, "Power %d", power);
    txt(c, 266, y + 46, b);
  }
  pd_text_fit(c, 12, 346, 488, "Up/Down:Mon Left/Right:Move",
              0x181818, PD_NO_SHADOW);
  pd_text_fit(c, 12, 366, 488,
      page ? "A:Info Y:Page Select:More B:Back" : "A:Info Y:Page B:Party",
      0x181818, PD_NO_SHADOW);
}
static void pc_move_draw(struct pd_canvas *c, const struct pd_mon *m, int slot,
                         const unsigned char *rom, size_t n, int build) {
  header(c, PC_PARTY);
  int id = m->moves[slot];
  bool change =
      build_ok(build) && builds[build].faithful && faithfulMoves[id].name;
  const char *name = change ? faithfulMoves[id].name : moves[id].name;
  int type = change ? faithfulMoves[id].type : moves[id].type,
      power = change ? faithfulMoves[id].power : moves[id].power;
  pd_text_fit(c, 20, 52, 472, name ? name : "Unknown move", 0x181818,
              PD_NO_SHADOW);
  char info[128], description[1024];
  snprintf(info, sizeof info, "%s Power %d PP %d",
           typeNames[type] ? typeNames[type] : "?", power, m->pp[slot]);
  pd_text_fit(c, 20, 88, 472, info, 0x181818, PD_NO_SHADOW);
  if (!table_text(rom, n, build, 22, id - 1, description, sizeof description))
    snprintf(description, sizeof description, "Move text needs matching ROM.");
  wrapped(c, 20, 128, 472, 9, description);
  txt(c, 12, 366, "B:Summary X:Play");
}
