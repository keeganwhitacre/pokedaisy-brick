static void pc_map_draw(struct pc_ui *, struct pd_canvas *,
                        const struct pd_snapshot *, const unsigned char *,
                        size_t, int);
static void map_center(struct pc_ui *, const struct pd_snapshot *);
/* GPL-3.0. Native adaptation of Gen2Reader.kt / Gen2Dex.kt.
 * Included by polished.c to share its validated ROM codecs and original tables.
 * Every ROM read is bounded; no art or descriptive text is shipped here. */
int pc_dex_count(void) { return (int)(sizeof dexOrder / sizeof *dexOrder); }
static int species_id(unsigned species, unsigned form) {
  int id = (int)species | ((form & 32) ? 256 : 0);
  for (unsigned k = 0; k < sizeof variants / sizeof *variants; k++)
    if (variants[k] == ((id << 8) | (int)(form & 31)))
      return 292 + (int)k;
  return id;
}
static bool battle_mon(struct pd_battle_mon *m, const unsigned char *p) {
  memset(m, 0, sizeof *m);
  int id = species_id(p[0], p[10]);
  if (!p[0] || p[0] == 255 || id >= 512 || !names[id] || !p[16] ||
      p[16] > 100 || be16(p + 19) > be16(p + 21) || !be16(p + 21) ||
      be16(p + 21) > 999 || p[33] >= 20 || p[34] >= 20)
    return false;
  m->species = id;
  m->level = p[16];
  m->status = p[17];
  m->hp = be16(p + 19);
  m->maxHp = be16(p + 21);
  m->heldItem = p[1];
  for (int i = 0; i < 5; i++)
    m->stats[i] = be16(p + 23 + 2 * i);
  m->type1 = p[33];
  m->type2 = p[34];
  for (int i = 0; i < 4; i++) {
    m->moves[i] = p[2 + i];
    m->pp[i] = p[11 + i] & 63;
  }
  return true;
}
static void read_sections(struct pd_snapshot *s, pd_read_fn read, void *ctx) {
  unsigned char map[4], backup[2], gender;
  s->posOk = read(ctx, PC_RAM_mapGroup, map, sizeof map) &&
             read(ctx, PC_RAM_backupMapGroup, backup, sizeof backup);
  if (s->posOk) {
    s->mapGroup = map[0];
    s->mapNum = map[1];
    s->y = map[2];
    s->x = map[3];
    s->mapsec = landmark(map[0] * 256u + map[1]);
    if (!s->mapsec)
      s->mapsec = landmark(backup[0] * 256u + backup[1]);
  }
  s->gender = read(ctx, PC_RAM_playerGender, &gender, 1) ? (gender & 1) : -1;
  unsigned char seen[37], caught[37], mode = 0, raw[35];
  s->dexNational = true;
  s->dexOk = read(ctx, PC_RAM_dexSeen, seen, sizeof seen) &&
             read(ctx, PC_RAM_dexCaught, caught, sizeof caught);
  if (s->dexOk)
    for (int i = 0; i < pc_dex_count(); i++) {
      int bit = dexOrder[i] - 1;
      if (bit < 0 || bit >= 291)
        continue;
      bool c = (caught[bit / 8] >> (bit % 8)) & 1;
      if (c)
        s->dexCaught[i / 8] |= 1u << (i % 8);
      if (c || ((seen[bit / 8] >> (bit % 8)) & 1))
        s->dexSeen[i / 8] |= 1u << (i % 8);
    }
  unsigned char badges[2], trades[2], tod;
  s->pcProgressOk =
      read(ctx, 0x10000u | PC_EVENT_FLAGS, s->flags, PC_EVENT_FLAG_BYTES) &&
      read(ctx, 0x10000u | PC_BADGES, badges, sizeof badges) &&
      read(ctx, 0x10000u | PC_TRADE_FLAGS, trades, sizeof trades) &&
      read(ctx, 0x10000u | PC_TIME_OF_DAY, &tod, 1);
  if (s->pcProgressOk) {
    s->pcBadges = badges[0] | (badges[1] << 8);
    s->pcTrades = trades[0] | (trades[1] << 8);
    s->pcTimeOfDay = tod <= PC_EVE ? tod : PC_DAY;
  }
  s->foeActive = s->foeNext = -1;
  if (!read(ctx, PC_RAM_battleMode, &mode, 1) || (mode != 1 && mode != 2))
    return;
  s->inBattle = true;
  s->isTrainer = mode == 2;
  unsigned char partyIndex;
  if (read(ctx, PC_RAM_curBattleMon, &partyIndex, 1) &&
      partyIndex < s->partyCount)
    s->partyActive = partyIndex;
  if (read(ctx, PC_RAM_battleMon, raw, sizeof raw))
    battle_mon(&s->battlers[PD_POS_PLAYER_LEFT], raw);
  if (read(ctx, PC_RAM_enemyMon, raw, sizeof raw))
    battle_mon(&s->battlers[PD_POS_OPPONENT_LEFT], raw);
  if (s->isTrainer) {
    unsigned char count = 0, active = 255, party[6 * 48];
    if (read(ctx, PC_RAM_otPartyCount, &count, 1) && count <= 6 &&
        read(ctx, PC_RAM_otPartyMons, party, sizeof party)) {
      s->foeCount = count;
      for (int i = 0; i < count; i++) {
        const unsigned char *p = party + 48 * i;
        struct pd_mon *m = &s->foes[i];
        m->species = species_id(p[0], p[21]);
        m->level = p[31];
        m->hp = be16(p + 34);
        m->maxHp = be16(p + 36);
        m->status = p[32];
        m->heldItem = p[1];
        for (int k = 0; k < 4; k++) {
          m->moves[k] = p[2 + k];
          m->pp[k] = p[22 + k] & 63;
        }
        if (!p[0] || p[0] == 255 || m->species >= 512 || !names[m->species] ||
            m->level < 1 || m->level > 100 || m->hp > m->maxHp ||
            m->maxHp > 999) {
          s->foeCount = 0;
          break;
        }
      }
      if (read(ctx, PC_RAM_curOTMon, &active, 1) && active < s->foeCount)
        s->foeActive = active;
    }
  }
}
static bool build_ok(int build) {
  return build >= 0 && build < (int)(sizeof builds / sizeof *builds);
}
// Gen2Dex.flow: join broken words, keeping the original compound-word hyphen.
static void rom_flow(char *text) {
  size_t read = 0, write = 0;
  while (text[read]) {
    if (text[read] == '\n') {
      while (text[read] == '\n' || text[read] == ' ')
        read++;
      bool join = write >= 2 && text[write - 1] == '-' &&
                  ((text[write - 2] >= 'A' && text[write - 2] <= 'z') ||
                   (unsigned char)text[write - 2] >= 128) &&
                  text[read] >= 'a' && text[read] <= 'z';
      if (join) {
        size_t len = 0;
        while ((text[read + len] >= 'a' && text[read + len] <= 'z') ||
               (text[read + len] >= 'A' && text[read + len] <= 'Z'))
          len++;
        bool compound = false;
        for (unsigned i = 0; i < sizeof compoundEnds / sizeof *compoundEnds;
             i++)
          if (strlen(compoundEnds[i]) == len &&
              !strncmp(compoundEnds[i], text + read, len))
            compound = true;
        if (!compound)
          write--;
      } else if (write && text[write - 1] != ' ')
        text[write++] = ' ';
      continue;
    }
    if (text[read] != ' ' || (write && text[write - 1] != ' '))
      text[write++] = text[read];
    read++;
  }
  while (write && text[write - 1] == ' ')
    write--;
  text[write] = 0;
}
/* ReadHuffmanChar, matching Gen2Dex.polishedString. Limits also reject cycles
 * and corrupt input instead of hanging the application on bad text. */
static bool rom_string(const unsigned char *rom, size_t n, size_t *pos,
                       int build, char *out, size_t cap) {
  if (!rom || !cap || !build_ok(build))
    return false;
  size_t o = *pos, used = 0, tree = off(builds[build].at[25]);
  unsigned bits = 0, byte = 0;
  bool packed = false;
  out[0] = 0;
  for (int count = 0; count < 4096; count++) {
    unsigned ch;
    if (!packed) {
      if (o >= n)
        return false;
      ch = rom[o++];
      if (ch == 0x5d) {
        packed = true;
        continue;
      }
    } else {
      unsigned node = 0, depth = 0;
      do {
        if (++depth > 64)
          return false;
        if (!bits) {
          if (o >= n)
            return false;
          byte = rom[o++];
          bits = 8;
        }
        unsigned bit = (byte >> 7) & 1;
        byte = (byte << 1) & 255;
        bits--;
        size_t at = tree + node * 2 + bit;
        if (at >= n)
          return false;
        node = rom[at];
      } while (node < 0x7f);
      ch = node >= 0xec ? node - 0xec + 0x4d : node;
    }
    if (ch == 0x52 || ch == 0x53 || (packed && ch == 0x54)) {
      while (used && out[used - 1] == ' ')
        used--;
      out[used] = 0;
      rom_flow(out);
      *pos = o;
      return true;
    }
    const char *text = ch >= 0x55 && ch <= 0x59 ? "\n" : textChars[ch];
    for (size_t k = 0; text[k]; k++) {
      if (text[k] == ' ' && (!used || out[used - 1] == ' '))
        continue;
      if (used + 1 >= cap)
        return false;
      out[used++] = text[k];
      out[used] = 0;
    }
  }
  return false;
}
static bool table_text(const unsigned char *rom, size_t n, int build, int table,
                       int index, char *out, size_t cap) {
  if (!rom || !build_ok(build) || index < 0)
    return false;
  unsigned at = builds[build].at[table];
  size_t p = off(at) + 2u * index;
  if (p + 2 > n)
    return false;
  size_t start = off((at & 0xff0000) | rom[p] | (rom[p + 1] << 8));
  return rom_string(rom, n, &start, build, out, cap);
}
static void wrapped(struct pd_canvas *c, int x, int y, int width, int lines,
                    const char *text) {
  pd_text_wrap(c, x, y, width, 22, lines, text, 0x181818, PD_NO_SHADOW);
}
static void front_pic(struct pd_canvas *c, int x, int y, int species,
                      const unsigned char *rom, size_t n, int build) {
  if (!rom || !build_ok(build) || species < 1 || species >= 512)
    return;
  const unsigned *a = builds[build].at;
  int i = species - 1;
  size_t sizeAt = off(a[14]) + i / 2, e = off(a[13]) + 7 * i,
         pal = off(a[2]) + 8 * species;
  if (sizeAt >= n || e + 3 > n || pal + 4 > n)
    return;
  int size = i % 2 ? rom[sizeAt] & 15 : rom[sizeAt] >> 4;
  if (size < 1 || size > 7)
    return;
  unsigned char tiles[0x4000];
  unsigned ptr = (rom[e] << 16) | rom[e + 1] | (rom[e + 2] << 8);
  if (lz(rom, n, off(ptr), tiles, sizeof tiles) < size * size * 16)
    return;
  uint32_t colors[] = {0xffffff, rgb(rom + pal), rgb(rom + pal + 2), 0};
  int ox = ((7 - size + 1) / 2) * 8, oy = (7 - size) * 8;
  for (int tile = 0; tile < size * size; tile++)
    for (int yy = 0; yy < 8; yy++)
      for (int xx = 0; xx < 8; xx++) {
        int v = ((tiles[16 * tile + 2 * yy] >> (7 - xx)) & 1) |
                (((tiles[16 * tile + 2 * yy + 1] >> (7 - xx)) & 1) << 1);
        if (v)
          pd_fill(c, x + (ox + tile / size * 8 + xx) * 2,
                  y + (oy + tile % size * 8 + yy) * 2, 2, 2, colors[v]);
      }
}
void pc_battle_draw(struct pd_canvas *c, const struct pd_snapshot *s,
                    const unsigned char *rom, size_t n, int build) {
  header(c, PC_BATTLE);
  if (!s->inBattle) {
    txt(c, 20, 70, "No battle in progress.");
    txt(c, 12, 359, "L/R:Section B/X:Play");
    return;
  }
  txt(c, 12, 36, s->isTrainer ? "TRAINER BATTLE" : "WILD BATTLE");
  for (int slot = 0; slot < 2; slot++) {
    const struct pd_battle_mon *m = &s->battlers[slot];
    int y = 62 + slot * 132;
    pd_fill(c, 8, y, 496, 124, 0x181818);
    pd_fill(c, 10, y + 2, 492, 120, 0xffffff);
    if (!m->species) {
      txt(c, 20, y + 10, "Battle data is not ready.");
      continue;
    }
    icon(c, 18, y + 12, m->species, rom, n, build);
    char b[96];
    snprintf(b, sizeof b, "%s Lv%d", names[m->species], m->level);
    pd_text_fit(c, 58, y + 7, 210, b, 0x181818, PD_NO_SHADOW);
    snprintf(b, sizeof b, "HP %d/%d %s", m->hp, m->maxHp,
             pd_status_label(m->status));
    txt(c, 58, y + 30, b);
    pd_fill(c, 58, y + 54, 180, 10, 0x181818);
    if (m->maxHp)
      pd_fill(c, 60, y + 56, 176 * m->hp / m->maxHp, 6,
              m->hp * 2 < m->maxHp ? 0xffbd00 : 0x00bd00);
    snprintf(b, sizeof b, "%s%s%s",
             typeNames[m->type1] ? typeNames[m->type1] : "?",
             m->type1 == m->type2 ? "" : "/",
             m->type1 == m->type2
                 ? ""
                 : (typeNames[m->type2] ? typeNames[m->type2] : "?"));
    txt(c, 20, y + 82, b);
    for (int k = 0; k < 4; k++)
      if (m->moves[k]) {
        int id = m->moves[k];
        const char *name =
            build_ok(build) && builds[build].faithful && faithfulMoves[id].name
                ? faithfulMoves[id].name
                : moves[id].name;
        pd_text_fit(c, 270, y + 8 + k * 26, 152, name ? name : "?", 0x181818,
                    PD_NO_SHADOW);
        snprintf(b, sizeof b, "%dPP", m->pp[k]);
        pd_text_right(c, 492, y + 8 + k * 26, b, 0x181818, PD_NO_SHADOW);
      }
  }
  char b[80];
  if (s->isTrainer) {
    snprintf(b, sizeof b, "Foe party:%d Active:%d", s->foeCount,
             s->foeActive + 1);
    txt(c, 12, 332, b);
  }
  txt(c, 12, 359, "L/R:Section B/X:Play");
}
/* Same ALL/CAUGHT/SEEN ONLY/NOT SEEN predicates as PokedexScreen.kt. */
static int dex_entries(const struct pd_snapshot *s, int filter, int ids[512]) {
  int count = 0;
  if (!s->dexOk)
    return 0;
  for (int i = 0; i < pc_dex_count(); i++) {
    bool seen = pd_dex_seen(s, i + 1), caught = pd_dex_caught(s, i + 1);
    if (filter == 0 || (filter == 1 && caught) ||
        (filter == 2 && seen && !caught) || (filter == 3 && !seen))
      ids[count++] = i;
  }
  return count;
}
static int dex_position(int selected, const int *ids, int count) {
  for (int i = 0; i < count; i++)
    if (ids[i] == selected)
      return i;
  return 0;
}
static void pc_dex_filtered_draw(struct pd_canvas *c,
                                 const struct pd_snapshot *s, int selected,
                                 bool detail, const unsigned char *rom,
                                 size_t n, int build, int filter) {
  header(c, PC_DEX);
  int count = pc_dex_count();
  if (selected < 0)
    selected = 0;
  if (selected >= count)
    selected = count - 1;
  if (!s->dexOk) {
    txt(c, 20, 70, "Pokedex data is not ready.");
    return;
  }
  int seen = 0, caught = 0;
  for (int i = 1; i <= count; i++) {
    seen += pd_dex_seen(s, i);
    caught += pd_dex_caught(s, i);
  }
  char b[128];
  snprintf(b, sizeof b, "Seen %d   Caught %d   Total %d", seen, caught, count);
  txt(c, 12, 36, b);
  if (!detail) {
    int ids[512], visible = dex_entries(s, filter, ids),
                  pos = dex_position(selected, ids, visible);
    int scroll = pos >= 11 ? pos - 10 : 0;
    if (!visible)
      txt(c, 20, 80, "No entries match this filter.");
    for (int row = 0; row < 11 && scroll + row < visible; row++) {
      int i = ids[scroll + row], id = dexOrder[i], y = 64 + row * 24;
      if (i == selected)
        pd_fill(c, 8, y, 496, 23, 0xdcdcdc);
      snprintf(b, sizeof b, "%s %03d  %s", i == selected ? ">" : " ", i + 1,
               pd_dex_seen(s, i + 1) ? names[id] : "----------");
      pd_text_fit(c, 16, y + 2, 368, b, 0x181818, PD_NO_SHADOW);
      pd_text_right(c, 492, y + 2,
                    pd_dex_caught(s, i + 1) ? "CAUGHT"
                    : pd_dex_seen(s, i + 1) ? "SEEN"
                                            : "",
                    0x181818, PD_NO_SHADOW);
    }
    const char *filters[] = {"ALL", "CAUGHT", "SEEN ONLY", "NOT SEEN"};
    snprintf(b, sizeof b, "Y:%s (%d)  A:Entry", filters[filter], visible);
    txt(c, 12, 346, b);
    txt(c, 12, 366, "L/R:Section B/X:Play");
    return;
  }
  int id = dexOrder[selected];
  snprintf(b, sizeof b, "No.%03d  %s", selected + 1,
           pd_dex_seen(s, selected + 1) ? names[id] : "----------");
  txt(c, 12, 62, b);
  if (!pd_dex_seen(s, selected + 1)) {
    txt(c, 20, 104, "Not seen yet.");
  } else if (!rom || !build_ok(build)) {
    txt(c, 20, 104, "Entry needs matching ROM.");
  } else {
    const unsigned *a = builds[build].at;
    front_pic(c, 20, 88, id, rom, n, build);
    size_t stats = off(a[15]) + 34u * (id - 1),
           body = off(a[12]) + 4u * (id - 1), e = off(a[11]) + 3u * (id - 1);
    if (stats + 16 <= n && body + 3 <= n && e + 3 <= n) {
      const char *labels[] = {"HP",      "ATTACK",  "DEFENSE",
                              "SP. ATK", "SP. DEF", "SPEED"};
      int order[] = {0, 1, 2, 4, 5, 3};
      for (int k = 0; k < 6; k++) {
        snprintf(b, sizeof b, "%s  %d", labels[k], rom[stats + order[k]]);
        txt(c, 160, 86 + k * 19, b);
      }
      snprintf(b, sizeof b, "%d.%dm   %d.%dkg", rom[body] / 10, rom[body] % 10,
               (rom[body + 1] + 256 * rom[body + 2]) / 10,
               (rom[body + 1] + 256 * rom[body + 2]) % 10);
      txt(c, 20, 204, b);
      size_t pos = off((rom[e] << 16) | rom[e + 1] | (rom[e + 2] << 8));
      char category[128], p1[768], p2[768], text[1600];
      if (rom_string(rom, n, &pos, build, category, sizeof category)) {
        pd_text_fit(c, 20, 228, 470, category, 0x181818, PD_NO_SHADOW);
        if (rom_string(rom, n, &pos, build, p1, sizeof p1) &&
            rom_string(rom, n, &pos, build, p2, sizeof p2)) {
          snprintf(text, sizeof text, "%s %s", p1, p2);
          wrapped(c, 20, 252, 470, 4, text);
        }
      }
    } else {
      txt(c, 20, 104, "Entry needs matching ROM.");
    }
  }
  txt(c, 12, 346, "Up/Down:Mon Y:Text");
  txt(c, 12, 366, "L/R:Section X:Play");
}
void pc_dex_draw(struct pd_canvas *c, const struct pd_snapshot *s, int selected,
                 bool detail, const unsigned char *rom, size_t n, int build) {
  pc_dex_filtered_draw(c, s, selected, detail, rom, n, build, 0);
}
static void pc_dex_text_draw(struct pd_canvas *c, const struct pd_snapshot *s,
                             int selected, const unsigned char *rom, size_t n,
                             int build) {
  header(c, PC_DEX);
  int id = dexOrder[selected];
  char title[128];
  snprintf(title, sizeof title, "No.%03d %s", selected + 1,
           pd_dex_seen(s, selected + 1) ? names[id] : "----------");
  pd_text_fit(c, 12, 40, 488, title, 0x181818, PD_NO_SHADOW);
  if (pd_dex_seen(s, selected + 1) && rom && build_ok(build)) {
    const unsigned *a = builds[build].at;
    size_t e = off(a[11]) + 3u * (id - 1);
    if (e + 3 <= n) {
      size_t pos = off((rom[e] << 16) | rom[e + 1] | (rom[e + 2] << 8));
      char category[128], p1[768], p2[768], text[1600];
      if (rom_string(rom, n, &pos, build, category, sizeof category) &&
          rom_string(rom, n, &pos, build, p1, sizeof p1) &&
          rom_string(rom, n, &pos, build, p2, sizeof p2)) {
        snprintf(text, sizeof text, "%s %s", p1, p2);
        wrapped(c, 16, 76, 480, 8, text);
      }
      size_t at = off(a[15]) + 34u * (id - 1);
      if (at + 16 <= n) {
        char ability[96];
        if (rom[at + 13] && table_text(rom, n, build, 23, rom[at + 13], ability,
                                       sizeof ability)) {
          snprintf(title, sizeof title, "Ability: %s", ability);
          pd_text_fit(c, 16, 266, 480, title, 0x181818, PD_NO_SHADOW);
        }
        if (rom[at + 14] && rom[at + 14] != rom[at + 13] &&
            table_text(rom, n, build, 23, rom[at + 14], ability,
                       sizeof ability)) {
          snprintf(title, sizeof title, "Ability: %s", ability);
          pd_text_fit(c, 16, 290, 480, title, 0x181818, PD_NO_SHADOW);
        }
        if (rom[at + 15] && table_text(rom, n, build, 23, rom[at + 15], ability,
                                       sizeof ability)) {
          snprintf(title, sizeof title, "Hidden: %s", ability);
          pd_text_fit(c, 16, 314, 480, title, 0x181818, PD_NO_SHADOW);
        }
      }
    }
  } else
    txt(c, 16, 90, "Entry is not available.");
  txt(c, 12, 346, "Up/Down:Mon Y:Picture");
  txt(c, 12, 366, "B:List X:Play");
}
void pc_item_draw(struct pd_canvas *c, int item, const unsigned char *rom,
                  size_t n, int build) {
  header(c, PC_BAG);
  txt(c, 20, 50, item_name(item, build));
  char text[1024] = {0};
  int table = 20, index = item - 1;
  if (item > 0x200) {
    table = 21;
    index = item - 0x201;
  } else if (item > 0x100) {
    table = 22;
    index = -1;
    if (rom && build_ok(build))
      for (unsigned i = 0; i < sizeof tmItems / sizeof *tmItems; i++)
        if (tmItems[i] == item) {
          size_t p = off(builds[build].at[24]) + i;
          if (p < n && rom[p])
            index = rom[p] - 1;
          break;
        }
  }
  if (!table_text(rom, n, build, table, index, text, sizeof text))
    snprintf(text, sizeof text, "Description needs matching ROM.");
  wrapped(c, 20, 88, 470, 11, text);
  txt(c, 12, 366, "B:Bag X:Play");
}

static void pc_party_draw(struct pd_canvas *, const struct pd_snapshot *, int,
                          bool, const unsigned char *, size_t, int, int);
static const int *mon_types(int, int);
static int type_percent(int, int);
#include "polished_battle.inc"
static int clamp_selection(int value, int count);
#include "polished_guide.inc"

static int clamp_selection(int value, int count) {
  return value < 0 || !count ? 0 : value >= count ? count - 1 : value;
}
void pc_ui_tick(struct pc_ui *u, uint32_t ms) { u->animationMs = ms; }
bool pc_ui_input(struct pc_ui *u, const struct pd_snapshot *s,
                 enum pc_action a) {
  if (a == PC_PREVIOUS || a == PC_NEXT) {
    u->section =
        (u->section + PC_SECTIONS + (a == PC_NEXT ? 1 : -1)) % PC_SECTIONS;
    u->summary = u->detail = u->moveOpen = u->placesOpen = false;
    return true;
  }
  if (a == PC_BACK) {
    if (u->section == PC_BATTLE && (u->battlePage || u->battleFoe)) {
      u->battlePage = 0;
      u->battleFoe = 0;
      return true;
    }
    if (u->section == PC_GUIDE) {
      if (u->guideReveal) {
        u->guideReveal = u->guideScroll = 0;
        return true;
      }
      if (u->guideBossOpen) {
        u->guideBossOpen = false;
        u->guideMon = 0;
        return true;
      }
    }
    if (u->placesOpen) {
      u->placesOpen = false;
      return true;
    }
    if (u->moveOpen)
      u->moveOpen = false;
    else if (u->summary)
      u->summary = false;
    else if (u->detail)
      u->detail = false;
    else
      return false;
    return true;
  }
  if (u->section == PC_BATTLE) {
    if (a == PC_ALTERNATE)
      u->battlePage = (u->battlePage + 1) % 5;
    if (u->battlePage == 4) {
      if (a == PC_LEFT || a == PC_RIGHT) {
        u->battleType = (u->battleType + 3 + (a == PC_RIGHT ? 1 : -1)) % 3;
        u->battleRow = 0;
      }
      if (a == PC_UP && u->battleRow > 0)
        u->battleRow--;
      if (a == PC_DOWN && u->battleRow < 2)
        u->battleRow++;
    }
    if (u->battlePage == 3) {
      if (a == PC_UP || a == PC_LEFT)
        u->battleRow--;
      if (a == PC_DOWN || a == PC_RIGHT)
        u->battleRow++;
      u->battleRow = clamp_selection(u->battleRow, s->foeCount);
      if (a == PC_CONFIRM && s->foeCount) {
        u->battleFoe = u->battleRow + 1;
        u->battleSeen |= 1u << u->battleRow;
        u->battlePage = 2;
      }
    }
    if (a == PC_PLACES)
      u->battleFoe = 0;
  } else if (u->section == PC_GUIDE) {
    if (a == PC_LEFT || a == PC_RIGHT) {
      u->guidePage = (u->guidePage + 5 + (a == PC_RIGHT ? 1 : -1)) % 5;
      u->guideRow = u->guideReveal = u->guideScroll = u->guideMon = 0;
      u->guideBossOpen = false;
    }
    if (u->guidePage == 2 && u->guideBossOpen) {
      int boss = guide_pending_boss(s, u->guideRow);
      if (boss < 0 || boss != u->guideBoss) {
        u->guideBossOpen = false;
        u->guideReveal = 0;
        return true;
      }
      int count = guideBosses[boss].count;
      if (a == PC_ALTERNATE) {
        u->guideMon = (u->guideMon + 1) % count;
        u->guideReveal = u->guideScroll = 0;
      }
      if (a == PC_UP || a == PC_DOWN) {
        int *pos = u->guideReveal ? &u->guideScroll : &u->guideMon;
        *pos = clamp_selection(*pos + (a == PC_DOWN ? 1 : -1),
                               u->guideReveal ? 4097 : count);
      }
    } else if (a == PC_UP || a == PC_DOWN) {
      int *position = u->guideReveal ? &u->guideScroll : &u->guideRow;
      *position = clamp_selection(*position + (a == PC_DOWN ? 1 : -1), 4097);
    }
    if (a == PC_ALTERNATE && (u->guidePage == 0 || u->guidePage == 4)) {
      u->guideMethod = (u->guideMethod + 1) % (u->guidePage == 0 ? 8 : 3);
      u->guideRow = u->guideReveal = u->guideScroll = u->guideMon = 0;
      u->guideBossOpen = false;
    }
    if (a == PC_CONFIRM && u->guidePage == 2) {
      int boss = guide_pending_boss(s, u->guideRow);
      if (boss >= 0) {
        if (!u->guideBossOpen) {
          u->guideBossOpen = true;
          u->guideBoss = boss;
          u->guideMon = 0;
          u->guideReveal = 0;
        } else
          u->guideReveal = (u->guideReveal + 1) % 3;
        u->guideScroll = 0;
      }
    } else if (a == PC_CONFIRM && u->guidePage != 0) {
      int tip = clamp_selection(u->guideRow,
                                (int)(sizeof guideTips / sizeof *guideTips));
      int steps = u->guidePage == 3 && !*guideTips[tip].hint ? 2 : 3;
      u->guideReveal = (u->guideReveal + 1) % steps;
      u->guideScroll = 0;
    }
  } else if (u->section == PC_MAP) {
    if (!u->mapSet)
      map_center(u, s);
    if (a == PC_PLACES)
      u->placesOpen = !u->placesOpen;
    else if (u->placesOpen) {
      if (a == PC_UP)
        u->place--;
      if (a == PC_DOWN)
        u->place++;
      if (a == PC_LEFT)
        u->place -= 11;
      if (a == PC_RIGHT)
        u->place += 11;
      u->place =
          clamp_selection(u->place, (int)(sizeof places / sizeof *places) - 1);
      if (a == PC_CONFIRM) {
        int i = u->place + 1;
        u->region = places[i].region;
        u->mapX = places[i].x;
        u->mapY = places[i].y;
        u->placesOpen = false;
      }
    } else {
      if (a == PC_LEFT && u->mapX > 0)
        u->mapX--;
      if (a == PC_RIGHT && u->mapX < 19)
        u->mapX++;
      if (a == PC_UP && u->mapY > 0)
        u->mapY--;
      if (a == PC_DOWN && u->mapY < 17)
        u->mapY++;
      if (a == PC_CONFIRM)
        u->region = (u->region + 1) % 3;
      if (a == PC_ALTERNATE)
        map_center(u, s);
    }
  } else if (u->section == PC_PARTY) {
    if (u->summary && !u->moveOpen && s->partyCount) {
      if (a == PC_UP)
        u->selected = (u->selected + s->partyCount - 1) % s->partyCount;
      if (a == PC_DOWN)
        u->selected = (u->selected + 1) % s->partyCount;
      if (a == PC_LEFT)
        u->move = (u->move + 3) % 4;
      if (a == PC_RIGHT)
        u->move = (u->move + 1) % 4;
      if (a == PC_ALTERNATE) {
        u->summaryPage = (u->summaryPage + 1) % 4;
        u->typePage = 0;
      }
      if (a == PC_PLACES)
        u->typePage = (u->typePage + 1) % 3;
      if (a == PC_CONFIRM && s->party[u->selected].species != 255 &&
          s->party[u->selected].moves[u->move])
        u->moveOpen = true;
    } else if (!u->summary && a == PC_CONFIRM && s->valid && s->partyCount)
      u->summary = true;
    if (!u->summary) {
      if (a == PC_LEFT)
        u->selected--;
      if (a == PC_RIGHT)
        u->selected++;
      if (a == PC_UP)
        u->selected -= 2;
      if (a == PC_DOWN)
        u->selected += 2;
      u->selected = clamp_selection(u->selected, s->partyCount);
    }
  } else if (u->section == PC_BAG && !u->detail) {
    if (a == PC_LEFT || a == PC_RIGHT) {
      u->pocket = (u->pocket + PD_POCKET_COUNT + (a == PC_RIGHT ? 1 : -1)) %
                  PD_POCKET_COUNT;
      u->item = 0;
    }
    if (a == PC_UP)
      u->item--;
    if (a == PC_DOWN)
      u->item++;
    u->item = clamp_selection(u->item, s->bag[u->pocket].count);
    if (a == PC_CONFIRM && s->bag[u->pocket].count)
      u->detail = true;
  } else if (u->section == PC_DEX) {
    int ids[512];
    if (!u->detail && a == PC_ALTERNATE)
      u->dexFilter = (u->dexFilter + 1) % 4;
    int count = dex_entries(s, u->dexFilter, ids),
        pos = dex_position(u->dex, ids, count);
    if (a == PC_UP)
      pos--;
    if (a == PC_DOWN)
      pos++;
    if (!u->detail && a == PC_LEFT)
      pos -= 11;
    if (!u->detail && a == PC_RIGHT)
      pos += 11;
    if (count)
      u->dex = ids[clamp_selection(pos, count)];
    if (u->detail && a == PC_ALTERNATE)
      u->dexPage = 1 - u->dexPage;
    if (a == PC_CONFIRM && !u->detail && count) {
      u->detail = true;
      u->dexPage = 0;
    }
  }
  return true;
}
void pc_ui_draw(struct pc_ui *u, struct pd_canvas *c,
                const struct pd_snapshot *s, const unsigned char *rom, size_t n,
                int build) {
  pc_font(rom, n, build);
  if (u->selected >= s->partyCount) {
    u->selected = clamp_selection(u->selected, s->partyCount);
    u->summary = u->moveOpen = false;
  }
  u->item = clamp_selection(u->item, s->bag[u->pocket].count);
  if (u->section == PC_PARTY) {
    if (u->moveOpen && s->partyCount)
      pc_move_draw(c, &s->party[u->selected], u->move, rom, n, build);
    else if (u->summary && s->partyCount)
      pc_summary_draw(c, s, u->selected, u->move, u->summaryPage, u->typePage,
                      rom, n, build);
    else
      pc_party_draw(c, s, u->selected, false, rom, n, build,
                    ((u->animationMs / 167) % 2) ? 10 : 2);
  } else if (u->section == PC_BAG) {
    if (u->detail && s->bag[u->pocket].count)
      pc_item_draw(c, s->bag[u->pocket].items[u->item].id, rom, n, build);
    else
      pc_bag_draw(c, s, u->pocket, u->item, build);
  } else if (u->section == PC_BATTLE)
    pc_battle_pages(u, c, s, rom, n, build);
  else if (u->section == PC_DEX) {
    int ids[512], count = dex_entries(s, u->dexFilter, ids);
    if (count)
      u->dex = ids[dex_position(u->dex, ids, count)];
    else
      u->detail = false;
    if (u->detail && u->dexPage)
      pc_dex_text_draw(c, s, u->dex, rom, n, build);
    else
      pc_dex_filtered_draw(c, s, u->dex, u->detail, rom, n, build,
                           u->dexFilter);
  } else if (u->section == PC_MAP)
    pc_map_draw(u, c, s, rom, n, build);
  else if (u->section == PC_GUIDE)
    pc_guide_draw(u, c, s, rom, n, build);
  pd_font_set(NULL, 0);
}
