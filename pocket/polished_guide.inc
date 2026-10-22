/* GPL-3.0. Controller adaptation of GuideScreen, GuidePolishedCrystal and
 * GuideAreas. Uses only upstream-generated definitions and read-only telemetry.
 */
#define GUIDE_COUNT(a) ((int)(sizeof(a) / sizeof *(a)))
static bool guide_flag(const struct pd_snapshot *s, int flag) {
  if (!s->pcProgressOk || flag <= 0)
    return false;
  if (flag >= 0x20000 && flag < 0x20010)
    return (s->pcTrades >> (flag - 0x20000)) & 1;
  if (flag >= 0x10000 && flag < 0x10010)
    return (s->pcBadges >> (flag - 0x10000)) & 1;
  return flag < PC_EVENT_FLAG_BYTES * 8 &&
         ((s->flags[flag / 8] >> (flag % 8)) & 1);
}
static int guide_party_best(const struct pd_snapshot *s) {
  int best = 0;
  if (s->valid)
    for (int i = 0; i < s->partyCount; i++)
      if (s->party[i].species && s->party[i].species != 255 &&
          s->party[i].level > best)
        best = s->party[i].level;
  return best;
}
static int guide_badges(const struct pd_snapshot *s) {
  int n = 0;
  for (int b = 0; b < 16; b++)
    n += (s->pcBadges >> b) & 1;
  return n;
}
static int guide_level(int raw, int badges) {
  if (raw <= 100)
    return raw;
  if (badges < 0)
    badges = 0;
  if (badges > 16)
    badges = 16;
  int v = guideBadgeLevels[badges] + raw - PC_LEVEL_FROM_BADGES;
  return v < 2 ? 2 : v < 100 ? v : v < PC_LEVEL_FROM_BADGES ? 99 : 2;
}
struct guide_encounter {
  int species, min, max, percent, weight;
};
// Merge identical species, then round and sort, as PolishedCrystalWorld.merge.
static int guide_encounters(const struct pd_snapshot *s, int method,
                            struct guide_encounter rows[32]) {
  int count = 0, time = s->pcTimeOfDay == PC_EVE ? PC_NITE : s->pcTimeOfDay;
  for (int i = 0; i < GUIDE_COUNT(guideWild); i++) {
    if (guideWild[i].map != s->mapGroup * 256 + s->mapNum ||
        guideWild[i].method != method ||
        (guideWild[i].tod >= 0 && guideWild[i].tod != time))
      continue;
    int species = guideWild[i].species;
    if (!species)
      species = s->pcTimeOfDay == PC_MORN || s->pcTimeOfDay == PC_DAY
                    ? PC_SPECIES_CORSOLA
                    : PC_SPECIES_STARYU;
    int lo = guide_level(guideWild[i].level, guide_badges(s));
    int hi = guide_level(guideWild[i].level + (method == 1 ? 4 : 0),
                         guide_badges(s));
    int k;
    for (k = 0; k < count && rows[k].species != species; k++)
      ;
    if (k == count) {
      if (count == 32)
        continue;
      rows[count++] =
          (struct guide_encounter){.species = species, .min = lo, .max = hi};
    }
    if (lo < rows[k].min)
      rows[k].min = lo;
    if (hi > rows[k].max)
      rows[k].max = hi;
    rows[k].weight += guideWild[i].weight;
  }
  for (int i = 0; i < count; i++) {
    int div = method >= 2 && method <= 4 ? 256 : 100;
    rows[i].percent = (rows[i].weight * 100 + div / 2) / div;
  }
  for (int i = 1; i < count; i++) {
    struct guide_encounter v = rows[i];
    int j = i;
    while (j && rows[j - 1].percent < v.percent) {
      rows[j] = rows[j - 1];
      j--;
    }
    rows[j] = v;
  }
  return count;
}
static bool guide_caught(const struct pd_snapshot *s, int id) {
  if (id > 291 && id - 292 < GUIDE_COUNT(variants))
    id = variants[id - 292] >> 8;
  for (int i = 0; i < pc_dex_count(); i++)
    if (dexOrder[i] == id)
      return pd_dex_caught(s, i + 1);
  return false;
}
static bool guide_area_done(const struct pd_snapshot *s, int i) {
  if (guideAreas[i].flag)
    return guide_flag(s, guideAreas[i].flag);
  int k = guideAreas[i].kind, id = guideAreas[i].id;
  if (k == 3 || (k == 2 && id >= 259))
    return s->bagOk && pd_bag_has(s, id);
  return k == 4 && guide_caught(s, id);
}
static void guide_where_name(int i, int build, char *name, size_t len) {
  int id = guideAreas[i].id, kind = guideAreas[i].kind;
  snprintf(name, len, "%s%s",
           kind >= 4 ? (id > 0 && id < 512 && names[id] ? names[id] : "?")
                     : item_name(id, build),
           kind == 5 ? " (egg)" : "");
}
static int guide_where(int category, int build, int ids[1024]) {
  int count = 0;
  for (int i = 0; i < GUIDE_COUNT(guideAreas); i++) {
    int k = guideAreas[i].kind, id = guideAreas[i].id;
    const char *name = item_name(id, build);
    bool hm = !strncmp(name, "HM", 2);
    if (category == 0   ? !((k == 0 || k == 2 || k == 3) && hm)
        : category == 1 ? !(k == 3 && !hm)
                        : !(k == 4 || k == 5))
      continue;
    char title[128];
    guide_where_name(i, build, title, sizeof title);
    if (!strcmp(title, "?") || !strcmp(title, "Unknown item"))
      continue;
    int found = -1;
    for (int j = 0; j < count; j++) {
      char other[128];
      guide_where_name(ids[j], build, other, sizeof other);
      if (!strcmp(title, other)) {
        found = j;
        break;
      }
    }
    if (found >= 0) {
      if (guideAreas[i].area < guideAreas[ids[found]].area)
        ids[found] = i;
    } else if (count < 1024)
      ids[count++] = i;
  }
  for (int i = 1; i < count; i++) {
    int id = ids[i], j = i;
    char title[128];
    guide_where_name(id, build, title, sizeof title);
    while (j) {
      char other[128];
      guide_where_name(ids[j - 1], build, other, sizeof other);
      if (strcmp(other, title) <= 0)
        break;
      ids[j] = ids[j - 1];
      j--;
    }
    ids[j] = id;
  }
  return count;
}
static bool guide_boss_done(const struct pd_snapshot *s, int i) {
  int rule = guideBosses[i].rule;
  if (rule == 1)
    return guide_flag(s, PC_EVENT_BEAT_ELITE_FOUR) ||
           guide_flag(s, guideBosses[i].flag);
  if (rule == 2)
    return guide_flag(s, PC_EVENT_BEAT_ELITE_FOUR_AGAIN) ||
           (guide_badges(s) == 16 && guide_flag(s, guideBosses[i].flag));
  return guide_flag(s, guideBosses[i].flag);
}
// Scroll full guide prose at the original 16-pixel font; never shrink
// paragraphs.
static void guide_prose(struct pd_canvas *c, int y, const char *text,
                        int *scroll) {
  char lines[96][512] = {{0}};
  int count = 0;
  const char *p = text;
  while (*p && count < 96) {
    int len = 0;
    while (*p && *p != '\n') {
      while (*p == ' ')
        p++;
      if (!*p || *p == '\n')
        break;
      const char *end = p;
      while (*end && *end != ' ' && *end != '\n')
        end++;
      int bytes = (int)(end - p);
      if (len + bytes + 2 >= 512)
        break;
      char word[512];
      memcpy(word, p, bytes);
      word[bytes] = 0;
      if (len && pd_text_width(lines[count]) + 16 + pd_text_width(word) > 472)
        break;
      if (len)
        lines[count][len++] = ' ';
      memcpy(lines[count] + len, p, bytes);
      len += bytes;
      lines[count][len] = 0;
      p = end;
    }
    if (*p == '\n')
      p++;
    count++;
  }
  int visible = (332 - y) / 20;
  if (*scroll > count - visible)
    *scroll = count > visible ? count - visible : 0;
  for (int i = 0; i < visible && *scroll + i < count; i++)
    pd_text_fit(c, 20, y + 20 * i, 472, lines[*scroll + i], 0x181818,
                PD_NO_SHADOW);
}
static void pc_guide_draw(struct pc_ui *u, struct pd_canvas *c,
                          const struct pd_snapshot *s, const unsigned char *rom,
                          size_t n, int build) {
  header(c, PC_GUIDE);
  const char *pages[] = {"HERE: WILD", "HERE: FINDS", "NEXT BOSS", "TIPS",
                         "WHERE IS"};
  char b[2048];
  snprintf(b, sizeof b, "< %s >", pages[u->guidePage]);
  txt(c, 12, 36, b);
  if (!s->pcProgressOk || !s->posOk) {
    txt(c, 20, 90, "Guide data is not ready.");
    txt(c, 12, 366, "L/R:Section B/X:Play");
    return;
  }
  const char *times[] = {"MORNING", "DAY", "NIGHT", "EVENING"};
  const char *where = s->mapsec > 0 && s->mapsec < GUIDE_COUNT(places)
                          ? places[s->mapsec].name
                          : "Unknown area";
  snprintf(b, sizeof b, "%s - %d badges", times[s->pcTimeOfDay],
           guide_badges(s));
  txt(c, 12, 60, b);
  if (u->guidePage == 0) {
    const char *methods[] = {"GRASS / CAVE",  "SURFING",   "OLD ROD",
                             "GOOD ROD",      "SUPER ROD", "HEADBUTT",
                             "RARE HEADBUTT", "ROCK SMASH"};
    pd_text_fit(c, 12, 84, 488, where, 0x181818, PD_NO_SHADOW);
    txt(c, 12, 108, methods[u->guideMethod]);
    struct guide_encounter rows[32] = {{0}};
    int count = guide_encounters(s, u->guideMethod, rows);
    if (u->guideRow >= count)
      u->guideRow = count ? count - 1 : 0;
    int start = u->guideRow >= 7 ? u->guideRow - 6 : 0;
    for (int i = start; i < count && i < start + 7; i++) {
      int y = 140 + (i - start) * 26;
      if (i == u->guideRow)
        pd_fill(c, 8, y - 2, 496, 25, 0xdcdcdc);
      int id = rows[i].species;
      snprintf(b, sizeof b, "%s%s Lv%d-%d %d%%",
               guide_caught(s, id) ? "+ " : "  ",
               id > 0 && id < 512 && names[id] ? names[id] : "?", rows[i].min,
               rows[i].max, rows[i].percent);
      pd_text_fit(c, 16, y, 480, b, 0x181818, PD_NO_SHADOW);
    }
    if (!count)
      txt(c, 20, 150, "No encounters for this method.");
    txt(c, 12, 346, "Y:Method +:Caught Up/Down:List");
  } else if (u->guidePage == 1) {
    pd_text_fit(c, 12, 84, 488, where, 0x181818, PD_NO_SHADOW);
    int ids[1024], count = 0;
    for (int i = 0; i < GUIDE_COUNT(guideAreas) && count < 1024; i++)
      if (guideAreas[i].area == s->mapsec)
        ids[count++] = i;
    if (u->guideRow >= count)
      u->guideRow = count ? count - 1 : 0;
    if (!count)
      txt(c, 20, 140, "No area finds listed here.");
    else if (u->guideReveal) {
      int i = ids[u->guideRow], id = guideAreas[i].id;
      const char *name = guideAreas[i].kind >= 4
                             ? (id < 512 && names[id] ? names[id] : "?")
                             : item_name(id, build);
      snprintf(b, sizeof b, "%s%s", guide_area_done(s, i) ? "DONE: " : "",
               name);
      pd_text_fit(c, 20, 114, 472, b, 0x181818, PD_NO_SHADOW);
      if (u->guideReveal == 1) {
        const char *kinds[] = {
            "Lying around this area.",  "Hidden in this area.",
            "Someone here gives this.", "A key item from someone here.",
            "A gift Pokemon here.",     "An Egg from someone here.",
            "A Pokemon trade here."};
        snprintf(b, sizeof b, "%s Press A for the location.",
                 kinds[guideAreas[i].kind]);
      } else if (guideAreas[i].kind == 6) {
        int wants = guideAreas[i].wants;
        snprintf(b, sizeof b, "%s\nTrade for %s.",
                 *guideAreas[i].where ? guideAreas[i].where : where,
                 wants > 0 && wants < 512 && names[wants] ? names[wants] : "?");
      } else
        snprintf(b, sizeof b, "%s\nQuantity: %d",
                 *guideAreas[i].where ? guideAreas[i].where : where,
                 guideAreas[i].qty);
      guide_prose(c, 146, b, &u->guideScroll);
    } else {
      int start = u->guideRow >= 8 ? u->guideRow - 7 : 0;
      const char *kinds[] = {"ITEM",     "HIDDEN", "GIFT", "KEY",
                             "GIFT MON", "EGG",    "TRADE"};
      for (int row = 0; row < 8 && start + row < count; row++) {
        int i = ids[start + row], id = guideAreas[i].id, y = 116 + 26 * row;
        if (start + row == u->guideRow)
          pd_fill(c, 8, y - 2, 496, 25, 0xdcdcdc);
        const char *name = guideAreas[i].kind >= 4
                               ? (id < 512 && names[id] ? names[id] : "?")
                               : item_name(id, build);
        snprintf(b, sizeof b, "%s %s: %s", guide_area_done(s, i) ? "+" : " ",
                 kinds[guideAreas[i].kind], name);
        pd_text_fit(c, 16, y, 480, b, 0x181818, PD_NO_SHADOW);
      }
    }
    txt(c, 12, 346, "A:Hint/Answer +:Done");
  } else if (u->guidePage == 2) {
    int ids[32], count = 0;
    for (int i = 0; i < GUIDE_COUNT(guideBosses); i++)
      if (!guide_boss_done(s, i))
        ids[count++] = i;
    if (u->guideRow >= count)
      u->guideRow = count ? count - 1 : 0;
    if (!count)
      txt(c, 20, 110, "Every boss is beaten.");
    else if (u->guideReveal) {
      int i = ids[u->guideRow];
      pd_text_fit(c, 12, 88, 488, guideBosses[i].title, 0x181818, PD_NO_SHADOW);
      pd_text_fit(c, 12, 112, 488, guideBosses[i].where, 0x181818,
                  PD_NO_SHADOW);
      const struct guide_team_mon *team =
          build_ok(build) && builds[build].faithful ? guideBosses[i].faithful
                                                    : guideBosses[i].normal;
      size_t used = 0;
      for (int k = 0; k < guideBosses[i].count; k++) {
        int id = team[k].species;
        if (u->guideReveal == 1) {
          const int *types = mon_types(id, build);
          used +=
              snprintf(b + used, sizeof b - used, "Pokemon %d: Lv%d %s%s%s\n",
                       k + 1, guide_level(team[k].level, guide_badges(s)),
                       typeNames[types[1]], types[1] != types[2] ? " / " : "",
                       types[1] != types[2] ? typeNames[types[2]] : "");
        } else {
          used += snprintf(b + used, sizeof b - used, "%s Lv%d\n", names[id],
                           guide_level(team[k].level, guide_badges(s)));
          for (int j = 0; j < 4; j++)
            if (team[k].moves[j]) {
              int m = team[k].moves[j];
              const char *name = build_ok(build) && builds[build].faithful &&
                                         faithfulMoves[m].name
                                     ? faithfulMoves[m].name
                                     : moves[m].name;
              used += snprintf(b + used, sizeof b - used, "%s%s", j ? ", " : "",
                               name ? name : "?");
            }
          used += snprintf(b + used, sizeof b - used, "\nHolds: %s\n\n",
                           item_name(team[k].item, build));
        }
      }
      int best = guide_party_best(s), boss = 0;
      for (int k = 0; k < guideBosses[i].count; k++) {
        int lv = guide_level(team[k].level, guide_badges(s));
        if (lv > boss)
          boss = lv;
      }
      char compare[128];
      if (best)
        snprintf(compare, sizeof compare, "Your best Lv%d / Boss Lv%d (%+d)",
                 best, boss, best - boss);
      else
        snprintf(compare, sizeof compare, "Boss top Lv%d / Party unavailable",
                 boss);
      pd_text_fit(c, 12, 136, 488, compare, 0x181818, PD_NO_SHADOW);
      guide_prose(c, 164, b, &u->guideScroll);
    } else {
      int start = u->guideRow >= 9 ? u->guideRow - 8 : 0;
      for (int row = 0; row < 9 && start + row < count; row++) {
        int i = ids[start + row], y = 88 + 26 * row;
        if (start + row == u->guideRow)
          pd_fill(c, 8, y - 2, 496, 25, 0xdcdcdc);
        snprintf(b, sizeof b, "%s%s",
                 start + row ? "" : "NEXT: ", guideBosses[i].title);
        pd_text_fit(c, 16, y, 480, b, 0x181818, PD_NO_SHADOW);
      }
    }
    txt(c, 12, 346, "A:Types/Team Up/Down:Scroll");
  } else if (u->guidePage == 4) {
    const char *categories[] = {"HMs", "KEY ITEMS", "GIFT POKEMON"};
    int ids[1024], category = u->guideMethod % 3;
    int count = guide_where(category, build, ids);
    txt(c, 12, 84, categories[category]);
    if (u->guideRow >= count)
      u->guideRow = count ? count - 1 : 0;
    if (!count)
      txt(c, 20, 130, "No entries in this category.");
    else if (u->guideReveal) {
      int i = ids[u->guideRow];
      char title[128];
      guide_where_name(i, build, title, sizeof title);
      pd_text_fit(c, 20, 114, 472, title, 0x181818, PD_NO_SHADOW);
      int area = guideAreas[i].area;
      const char *place = area > 0 && area < GUIDE_COUNT(places)
                              ? places[area].name
                              : "Unknown area";
      if (u->guideReveal == 1)
        snprintf(b, sizeof b, "In %s.", place);
      else
        snprintf(b, sizeof b, "%s%s%s",
                 *guideAreas[i].where ? guideAreas[i].where
                                      : "Someone around this area.",
                 *guideAreas[i].where ? " - " : " ", place);
      guide_prose(c, 146, b, &u->guideScroll);
    } else {
      int start = u->guideRow >= 8 ? u->guideRow - 7 : 0;
      for (int row = 0; row < 8 && start + row < count; row++) {
        int i = ids[start + row], y = 116 + 26 * row;
        char title[128];
        guide_where_name(i, build, title, sizeof title);
        if (start + row == u->guideRow)
          pd_fill(c, 8, y - 2, 496, 25, 0xdcdcdc);
        snprintf(b, sizeof b, "%s %s", guide_area_done(s, i) ? "+" : " ",
                 title);
        pd_text_fit(c, 16, y, 480, b, 0x181818, PD_NO_SHADOW);
      }
    }
    txt(c, 12, 346, "Y:Category A:Hint/Answer");
  } else {
    int count = GUIDE_COUNT(guideTips);
    if (u->guideRow >= count)
      u->guideRow = count - 1;
    if (u->guideReveal) {
      snprintf(b, sizeof b, "%s\n\n%s", guideTips[u->guideRow].title,
               u->guideReveal == 1 && *guideTips[u->guideRow].hint
                   ? guideTips[u->guideRow].hint
                   : guideTips[u->guideRow].answer);
      guide_prose(c, 88, b, &u->guideScroll);
    } else {
      int start = u->guideRow >= 5 ? u->guideRow - 4 : 0;
      for (int i = start; i < count && i < start + 5; i++) {
        int y = 88 + 48 * (i - start);
        if (i == u->guideRow)
          pd_fill(c, 8, y - 2, 496, 46, 0xdcdcdc);
        wrapped(c, 16, y, 480, 2, guideTips[i].title);
      }
    }
    txt(c, 12, 346, "A:Hint/Answer Up/Down:Scroll");
  }
  txt(c, 12, 366, "Left/Right:Page B:Back X:Play");
  (void)rom;
  (void)n;
}
#undef GUIDE_COUNT
