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
// The source GuideScreen builds four pages of sections/entries, with one
// stable open key. Keep that model independent of the emulator and SDL.
enum guide_row_kind { G_HEADING, G_WILD, G_AREA, G_BOSS, G_TIP, G_WHERE };
struct guide_row {
  int kind, ref, aux;
  struct guide_encounter encounter;
  uint64_t key;
};
#define GUIDE_ROWS 1024
struct guide_entry {
  char title[256], detail[128], hint[512], answer[2048];
  int icon, steps;
  bool owned;
};
static bool guide_seen(const struct pd_snapshot *s, int id) {
  if (!s->dexOk)
    return true; // GuideScreen's dex == null rule.
  if (id > 291 && id - 292 < GUIDE_COUNT(variants))
    id = variants[id - 292] >> 8;
  for (int i = 0; i < pc_dex_count(); i++)
    if (dexOrder[i] == id)
      return pd_dex_seen(s, i + 1);
  return false;
}
static const char *guide_place(int area) {
  return area > 0 && area < GUIDE_COUNT(places) ? places[area].name
                                                : "THIS MAP";
}
static uint64_t guide_key(int kind, int ref, int aux) {
  return ((uint64_t)(kind + 1) << 56) | ((uint64_t)(unsigned)ref << 24) |
         (unsigned)aux;
}
static void guide_add(struct guide_row *rows, int *count, int kind, int ref,
                      int aux) {
  if (*count >= GUIDE_ROWS)
    return;
  rows[(*count)++] = (struct guide_row){
      .kind = kind, .ref = ref, .aux = aux, .key = guide_key(kind, ref, aux)};
}
// Heading refs: 0 area, 1..8 methods, 9 PEOPLE, 10 ITEMS, 11 TIPS,
// 12..14 WHERE IS categories, 100+ boss. Stable owned-last order is upstream.
static int guide_rows(const struct pd_snapshot *s, int page, int build,
                      struct guide_row rows[GUIDE_ROWS]) {
  int count = 0;
  if (page == 0) {
    guide_add(rows, &count, G_HEADING, 0, 0);
    for (int j = 0; j < 8; j++) {
      int method = pcGuideMethodOrder[j];
      struct guide_encounter enc[32] = {{0}};
      int n = guide_encounters(s, method, enc);
      if (n)
        guide_add(rows, &count, G_HEADING, method + 1, 0);
      for (int i = 0; i < n; i++) {
        guide_add(rows, &count, G_WILD, method, enc[i].species);
        rows[count - 1].encounter = enc[i];
        rows[count - 1].key = guide_key(G_WILD, s->mapGroup * 256 + s->mapNum,
                                        method * 512 + enc[i].species);
      }
    }
    for (int group = 0; group < 2; group++) {
      bool heading = false;
      for (int done = 0; done < 2; done++)
        for (int i = 0; i < GUIDE_COUNT(guideAreas); i++) {
          int k = guideAreas[i].kind;
          if (guideAreas[i].area != s->mapsec || (k <= 1) != (group == 1) ||
              guide_area_done(s, i) != done)
            continue;
          if (!heading) {
            guide_add(rows, &count, G_HEADING, group ? 10 : 9, 0);
            heading = true;
          }
          guide_add(rows, &count, G_AREA, i, 0);
        }
    }
  } else if (page == 2) {
    bool first = true;
    for (int i = 0; i < GUIDE_COUNT(guideBosses); i++)
      if (!guide_boss_done(s, i)) {
        guide_add(rows, &count, G_HEADING, 100 + i, first);
        first = false;
        for (int k = 0; k < guideBosses[i].count; k++)
          guide_add(rows, &count, G_BOSS, i, k);
      }
    if (!count)
      guide_add(rows, &count, G_HEADING, 15, 0);
  } else if (page == 3) {
    guide_add(rows, &count, G_HEADING, 11, 0);
    for (int i = 0; i < GUIDE_COUNT(guideTips); i++)
      guide_add(rows, &count, G_TIP, i, 0);
  } else {
    for (int cat = 0; cat < 3; cat++) {
      int ids[1024], n = guide_where(cat, build, ids);
      if (n)
        guide_add(rows, &count, G_HEADING, 12 + cat, 0);
      for (int j = 0; j < n; j++)
        guide_add(rows, &count, G_WHERE, ids[j], 0);
    }
  }
  return count;
}
static void guide_entry(const struct guide_row *r, const struct pd_snapshot *s,
                        int build, struct guide_entry *e) {
  memset(e, 0, sizeof *e);

  if (r->kind == G_HEADING) {
    int h = r->ref;
    const char *labels[] = {"PEOPLE",    "ITEMS",        "TIPS",    "HMs",
                            "KEY ITEMS", "GIFT POKéMON", "ALL DONE"};
    if (h == 0) {
      snprintf(e->title, sizeof e->title, "%s", guide_place(s->mapsec));
      bool any = false;
      bool species[512] = {0};
      int total = 0, caught = 0;
      for (int method = 0; method < 8; method++) {
        struct guide_encounter enc[32] = {{0}};
        int n = guide_encounters(s, method, enc);
        any |= n > 0;
        for (int i = 0; i < n; i++)
          if (!species[enc[i].species]) {
            species[enc[i].species] = true;
            total++;
            caught += guide_caught(s, enc[i].species);
          }
      }
      if (any && s->dexOk)
        snprintf(e->answer, sizeof e->answer, "%d of %d caught here. ", caught,
                 total);
      else if (!any)
        snprintf(e->answer, sizeof e->answer, "No wild POKéMON on this map. ");
      bool finds = false;
      for (int i = 0; i < GUIDE_COUNT(guideAreas); i++)
        finds |= guideAreas[i].area == s->mapsec;
      snprintf(e->answer + strlen(e->answer),
               sizeof e->answer - strlen(e->answer), "%sA ball = done.",
               finds ? "Its buildings included. " : "");
    } else if (h <= 8) {
      snprintf(e->title, sizeof e->title, "%s", pcGuideMethods[h - 1]);
      if (h == 1) {
        const char *times[] = {"MORNING", "DAY", "NIGHT", "EVENING"};
        snprintf(e->answer, sizeof e->answer, "For your game's clock: %s.",
                 times[s->pcTimeOfDay]);
      }
    } else if (h < 100) {
      snprintf(e->title, sizeof e->title, "%s", labels[h - 9]);
      if (h == 9)
        snprintf(e->answer, sizeof e->answer,
                 "Some want something done first.");
      if (h == 15)
        snprintf(e->answer, sizeof e->answer, "Every boss is beaten.");
    } else {
      int i = h - 100, best = guide_party_best(s), top = 0;
      const struct guide_team_mon *team =
          build_ok(build) && builds[build].faithful ? guideBosses[i].faithful
                                                    : guideBosses[i].normal;
      for (int k = 0; k < guideBosses[i].count; k++) {
        int level = guide_level(team[k].level, guide_badges(s));
        if (level > top)
          top = level;
      }
      snprintf(e->title, sizeof e->title, "%s%s", r->aux ? "NEXT: " : "",
               guideBosses[i].title);
      snprintf(e->answer, sizeof e->answer, "%s · %d POKéMON · UP TO LV %d",
               guideBosses[i].where, guideBosses[i].count, top);
      if (r->aux && best)
        snprintf(e->answer + strlen(e->answer),
                 sizeof e->answer - strlen(e->answer), " · YOUR BEST LV %d",
                 best);
    }
    return;
  }
  if (r->kind == G_WILD) {
    int id = r->encounter.species;
    bool seen = guide_seen(s, id);
    snprintf(e->title, sizeof e->title, "%s", seen ? names[id] : "???");
    if (!seen)
      snprintf(e->answer, sizeof e->answer, "%s", names[id]);
    if (r->encounter.min == r->encounter.max)
      snprintf(e->detail, sizeof e->detail, "LV %d  %d%%", r->encounter.min,
               r->encounter.percent);
    else
      snprintf(e->detail, sizeof e->detail, "LV %d-%d  %d%%", r->encounter.min,
               r->encounter.max, r->encounter.percent);
    e->icon = id;
    e->owned = guide_caught(s, id);
  } else if (r->kind == G_BOSS) {
    int i = r->ref, k = r->aux;
    const struct guide_team_mon *team =
        build_ok(build) && builds[build].faithful ? guideBosses[i].faithful
                                                  : guideBosses[i].normal;
    int id = team[k].species;
    const int *types = mon_types(id, build);
    snprintf(e->title, sizeof e->title, "POKéMON %d", k + 1);
    snprintf(e->detail, sizeof e->detail, "LV %d",
             guide_level(team[k].level, guide_badges(s)));
    snprintf(e->hint, sizeof e->hint, "%s%s%s TYPE", typeNames[types[1]],
             types[1] != types[2] ? " / " : "",
             types[1] != types[2] ? typeNames[types[2]] : "");
    snprintf(e->answer, sizeof e->answer, "%s", names[id]);
    for (int j = 0; j < 4; j++)
      if (team[k].moves[j]) {
        int m = team[k].moves[j];
        const char *name =
            build_ok(build) && builds[build].faithful && faithfulMoves[m].name
                ? faithfulMoves[m].name
                : moves[m].name;
        snprintf(e->answer + strlen(e->answer),
                 sizeof e->answer - strlen(e->answer), "%s%s", j ? " · " : "\n",
                 name ? name : "?");
      }
    if (team[k].item)
      snprintf(e->answer + strlen(e->answer),
               sizeof e->answer - strlen(e->answer), "\nHOLDS %s",
               item_name(team[k].item, build));
    e->icon = id;
  } else if (r->kind == G_TIP) {
    snprintf(e->title, sizeof e->title, "%s", guideTips[r->ref].title);
    snprintf(e->hint, sizeof e->hint, "%s", guideTips[r->ref].hint);
    snprintf(e->answer, sizeof e->answer, "%s", guideTips[r->ref].answer);
  } else {
    int i = r->ref, id = guideAreas[i].id, k = guideAreas[i].kind;
    e->owned = guide_area_done(s, i);
    const char *name = k >= 4 ? names[id] : item_name(id, build);
    char at[256], qty[24] = "";
    if (guideAreas[i].qty > 1)
      snprintf(qty, sizeof qty, " x%d", guideAreas[i].qty);
    if (*guideAreas[i].where)
      snprintf(at, sizeof at, "in the %s", guideAreas[i].where);
    else
      snprintf(at, sizeof at, "around %s", guide_place(guideAreas[i].area));
    snprintf(e->title, sizeof e->title, "%s%s", name ? name : "?", qty);
    if (r->kind == G_WHERE) {
      guide_where_name(i, build, e->title, sizeof e->title);
      snprintf(e->hint, sizeof e->hint, "In %s.",
               guide_place(guideAreas[i].area));
      if (*guideAreas[i].where)
        snprintf(e->answer, sizeof e->answer, "%s in the %s (%s).",
                 k == 0 ? "Lying" : "Someone", guideAreas[i].where,
                 guide_place(guideAreas[i].area));
      else
        snprintf(e->answer, sizeof e->answer, "%s around %s.",
                 k == 0 ? "Lying" : "Someone", guide_place(guideAreas[i].area));
    } else if (k == 1 && !e->owned) {
      snprintf(e->title, sizeof e->title, "HIDDEN ITEM");
      snprintf(e->hint, sizeof e->hint, "Buried %s.", at);
      snprintf(e->answer, sizeof e->answer, "%s%s", name, qty);
    } else if (k <= 1)
      snprintf(e->answer, sizeof e->answer, "%s %s.", k ? "Hidden" : "Lying",
               at);
    else if (k == 5) {
      snprintf(e->title, sizeof e->title, "AN EGG");
      snprintf(e->answer, sizeof e->answer,
               "Someone %s gives you an EGG. It hatches into %s.", at, name);
    } else if (k == 6) {
      snprintf(e->title, sizeof e->title, "TRADE FOR %s", name);
      snprintf(e->answer, sizeof e->answer, "Someone %s trades it for your %s.",
               at, names[guideAreas[i].wants]);
      e->icon = id;
    } else {
      snprintf(e->answer, sizeof e->answer,
               k == 4 ? "Someone %s gives you this POKéMON."
                      : "Someone %s gives it to you.",
               at);
      if (k == 4)
        e->icon = id;
    }
  }
  e->steps = *e->answer ? (*e->hint ? 2 : 1) : 0;
}
static int guide_first_entry(const struct guide_row *rows, int count, int start,
                             int dir) {
  for (int i = start; i >= 0 && i < count; i += dir)
    if (rows[i].kind != G_HEADING)
      return i;
  return -1;
}
static void guide_sync(struct pc_ui *u, const struct pd_snapshot *s, int build,
                       struct guide_row *rows, int *count) {
  *count = guide_rows(s, u->guidePage, build, rows);
  if (u->guideRow < 0 || u->guideRow >= *count ||
      rows[u->guideRow].kind == G_HEADING) {
    int first = guide_first_entry(rows, *count, 0, 1);
    u->guideRow = first < 0 ? 0 : first;
  }
  bool found = false;
  for (int i = 0; i < *count; i++)
    found |= rows[i].key == u->guideOpenKey;
  if (!found && u->guideOpenPage == u->guidePage) {
    u->guideOpenKey = 0;
    u->guideReveal = 0;
  }
}
static void guide_input(struct pc_ui *u, const struct pd_snapshot *s,
                        enum pc_action a) {
  const int pages[] = {0, 2, 3, 4};
  int page = 0;
  for (int i = 0; i < 4; i++)
    if (u->guidePage == pages[i])
      page = i;
  if (a == PC_LEFT) {
    u->guidePagesFocused = true;
    return;
  }
  if (a == PC_RIGHT || (a == PC_CONFIRM && u->guidePagesFocused)) {
    u->guidePagesFocused = false;
    return;
  }
  if (a == PC_ALTERNATE ||
      (u->guidePagesFocused && (a == PC_UP || a == PC_DOWN))) {
    page = (page + 4 + (a == PC_UP ? -1 : 1)) % 4;
    u->guidePage = pages[page];
    u->guideRow = u->guideScroll = u->guideRowScroll = 0;
    return;
  }
  struct guide_row rows[GUIDE_ROWS];
  int count;
  // Game build does not change the selected row in HERE/NEXT BOSS/TIPS.
  // WHERE IS ordering is identical for faithful; title spelling comes from
  // draw.
  guide_sync(u, s, u->guideBuild, rows, &count);
  if (a == PC_UP || a == PC_DOWN) {
    int next =
        guide_first_entry(rows, count, u->guideRow + (a == PC_DOWN ? 1 : -1),
                          a == PC_DOWN ? 1 : -1);
    if (next >= 0) {
      u->guideRow = next;
      u->guideRowScroll = 0;
    }
  }
  if (a == PC_PLACES)
    u->guideRowScroll =
        u->guideRowScroll >= u->guideRowScrollMax ? 0 : u->guideRowScroll + 180;
  if (a == PC_CONFIRM && count && rows[u->guideRow].kind != G_HEADING) {
    struct guide_entry e;
    guide_entry(&rows[u->guideRow], s, u->guideBuild, &e);
    if (e.steps) {
      if (u->guideOpenKey != rows[u->guideRow].key)
        u->guideReveal = 0;
      u->guideReveal = (u->guideReveal + 1) % (e.steps + 1);
      u->guideOpenKey = u->guideReveal ? rows[u->guideRow].key : 0;
      u->guideOpenPage = u->guidePage;
      u->guideRowScroll = 0;
    }
  }
}
// SectionHeading + EntryRow: same title/detail/tag, owned ball and hidden-icon
// rules as Compose. The 16px cart font stays readable; details wrap on Brick.
static int guide_row_draw(struct pd_canvas *c, int y, const struct guide_row *r,
                          const struct guide_entry *e, int level, bool selected,
                          const unsigned char *rom, size_t n, int build) {
  int top = y;
  if (r->kind == G_HEADING) {
    y += 12;
    y += 20 * pd_text_wrap(c, 206, y, 278, 20, 128, e->title, 0x181818,
                           PD_NO_SHADOW);
    if (*e->answer) {
      y += 4;
      y += 20 * pd_text_wrap(c, 206, y, 278, 20, 128, e->answer, 0x787878,
                             PD_NO_SHADOW);
    }
    return y - top + 8;
  }
  struct pd_canvas measure = *c;
  measure.cx1 = measure.cx0;
  if (selected) {
    int h = guide_row_draw(&measure, y, r, e, level, false, rom, n, build);
    pd_fill(c, 204, y, 288, h, 0xdcdcdc);
  }
  y += 8;
  int x = 208;
  bool answer = e->steps > 0 && level >= e->steps;
  if (e->owned) {
    dex_mark(c, x, y + 2, true, true);
    x += 18;
  }
  bool showIcon = e->icon && (!e->steps || answer);
  if (showIcon) {
    icon(c, x, y, e->icon, rom, n, build);
    x += 36;
  }
  int titleH = 20 * pd_text_wrap(c, x, y, 488 - x, 20, 128, e->title, 0x181818,
                                 PD_NO_SHADOW);
  y += showIcon && titleH < 32 ? 32 : titleH;
  if (*e->detail || (e->steps && !answer)) {
    y += 4;
    if (*e->detail)
      y += 20 * pd_text_wrap(c, 208, y, 278, 20, 128, e->detail, 0x181818,
                             PD_NO_SHADOW);
    if (e->steps && !answer) {
      pd_text(c, 208, y, *e->hint && !level ? "HINT" : "ANSWER", 0x787878,
              PD_NO_SHADOW);
      y += 20;
    }
  }
  if (*e->hint && level >= 1) {
    char hint[600];
    snprintf(hint, sizeof hint, "HINT: %s", e->hint);
    y += 4;
    y += 20 *
         pd_text_wrap(c, 208, y, 278, 20, 128, hint, 0x787878, PD_NO_SHADOW);
  }
  if (answer) {
    y += 4;
    y += 20 * pd_text_wrap(c, 208, y, 278, 20, 128, e->answer, 0x181818,
                           PD_NO_SHADOW);
  }
  pd_fill(c, 208, y + 6, 276, 2, 0xb0b0b0);
  return y - top + 14;
}
static void pc_guide_draw(struct pc_ui *u, struct pd_canvas *c,
                          const struct pd_snapshot *s, const unsigned char *rom,
                          size_t n, int build) {
  u->guideBuild = build;
  header(c, PC_GUIDE);
  pd_layered_box(c, 8, 38, 176, 44, 4, pcOptionFrame, 5, 0xffffff);
  txt(c, 22, 52, "GUIDE");
  pd_layered_box(c, 8, 86, 176, 248, 4, pcOptionFrame, 5, 0xffffff);
  const int pages[] = {0, 2, 3, 4};
  for (int i = 0; i < 4; i++) {
    int y = 102 + i * 48;
    if (u->guidePage == pages[i])
      pd_fill(c, 20, y, 152, 38, 0xdcdcdc);
    txt(c, 30, y + 12, pcGuidePages[i]);
    if (u->guidePagesFocused && u->guidePage == pages[i])
      pd_triangle(c, 20, y + 14, 6, PD_RIGHT, 0x181818);
  }
  pd_layered_box(c, 192, 38, 312, 296, 4, pcOptionFrame, 5, 0xffffff);
  if (!s->pcProgressOk || !s->posOk) {
    pd_text_wrap(c, 208, 60, 276, 20, 5, "Guide data is not ready.", 0x787878,
                 PD_NO_SHADOW);
    return;
  }
  struct guide_row rows[GUIDE_ROWS];
  int count;
  guide_sync(u, s, build, rows, &count);
  int heights[GUIDE_ROWS], total = 0, selectedTop = 0, selectedH = 0;
  struct pd_canvas measure = *c;
  measure.cx1 = measure.cx0;
  for (int i = 0; i < count; i++) {
    struct guide_entry e;
    guide_entry(&rows[i], s, build, &e);
    int level = rows[i].key == u->guideOpenKey ? u->guideReveal : 0;
    heights[i] =
        guide_row_draw(&measure, 0, &rows[i], &e, level, false, NULL, 0, -1);
    if (i == u->guideRow) {
      selectedTop = total;
      selectedH = heights[i];
    }
    total += heights[i];
  }
  const int visible = 272;
  int maxRowScroll = selectedH > visible ? selectedH - visible : 0;
  u->guideRowScrollMax = maxRowScroll;
  if (u->guideRowScroll > maxRowScroll)
    u->guideRowScroll = maxRowScroll;
  if (selectedH > visible)
    u->guideScroll = selectedTop + u->guideRowScroll;
  else if (selectedTop < u->guideScroll)
    u->guideScroll = selectedTop;
  else if (selectedTop + selectedH > u->guideScroll + visible)
    u->guideScroll = selectedTop + selectedH - visible;
  if (u->guideScroll > total - visible)
    u->guideScroll = total > visible ? total - visible : 0;
  if (u->guideScroll < 0)
    u->guideScroll = 0;
  pd_clip(c, 204, 50, 288, visible);
  int y = 50 - u->guideScroll;
  for (int i = 0; i < count; i++) {
    if (y + heights[i] > 50 && y < 322) {
      struct guide_entry e;
      guide_entry(&rows[i], s, build, &e);
      guide_row_draw(c, y, &rows[i], &e,
                     rows[i].key == u->guideOpenKey ? u->guideReveal : 0,
                     i == u->guideRow && !u->guidePagesFocused, rom, n, build);
    }
    y += heights[i];
  }
  pd_unclip(c);
  pd_text_fit(c, 12, 346, 488,
              u->guidePagesFocused ? "Up/Down:Page Right:Entries"
                                   : "A:Reveal Select:Scroll",
              0x181818, PD_NO_SHADOW);
  txt(c, 12, 366, "Left:Pages B:Back L/R:Tabs");
}
#undef GUIDE_COUNT
