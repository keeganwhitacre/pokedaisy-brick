/* GPL-3.0; adapted from PokeDaisy Gen2Reader.kt, Gen2Art.kt, Gen2Dex.kt.
 * ROM art is decoded only from the user's SHA-1 matched ROM. */
#include "polished.h"
#include "experience.h"
#include "pd_font.h"
#include "pixel_widgets.h"
#include "polished_guide_tables.h"
#include "polished_tables.h"
#include "rom_identity.h"
#include <stdio.h>
#include <string.h>
static const struct pocket_hp_palette polishedHp = {
    0x181818, 0xffffff, 0xffce10, 0x00bd00, 0xffbd00, 0xff0000};
static struct pd_glyph romFont[129];
static const unsigned char *fontRom;
static size_t fontSize;
static int fontBuild = -1;
void pc_font(const unsigned char *rom, size_t n, int build) {
  if (!rom || build < 0 || build >= (int)(sizeof builds / sizeof *builds)) {
    pd_font_set(NULL, 0);
    return;
  }
  if (rom != fontRom || n != fontSize || build != fontBuild) {
    memset(romFont, 0, sizeof romFont);
    unsigned a = builds[build].at[0];
    size_t base = (a >> 16) * 0x4000 + (a & 0x3fff);
    if (base + 1024 > n) {
      pd_font_set(NULL, 0);
      return;
    }
    for (int i = 0; i < 128; i++) {
      romFont[i].codepoint = fontCodepoints[i];
      romFont[i].advance = 16;
      for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
          if (rom[base + i * 8 + y] & (1u << (7 - x))) {
            romFont[i].rows[2 * y] |= 3u << (2 * x);
            romFont[i].rows[2 * y + 1] |= 3u << (2 * x);
          }
    }
    romFont[128].codepoint = ' ';
    romFont[128].advance = 16;
    fontRom = rom;
    fontSize = n;
    fontBuild = build;
  }
  pd_font_set(romFont, 129);
}
static bool pc_exp(const struct pd_mon *m, int width, struct pocket_exp *xp) {
  static const int rates[] = {0, 3, 4, 5};
  if (m->species <= 0 || m->species == 255 ||
      m->species >=
          (int)(sizeof polishedGrowthRates / sizeof *polishedGrowthRates))
    return false;
  int rate = polishedGrowthRates[m->species];
  return rate >= 0 && rate < 4 &&
         pocket_exp_progress(rates[rate], m->level, m->experience, width, xp);
}
static int landmark(unsigned map);
int pc_identify(const unsigned char *rom, size_t n) {
  char hash[41];
  pocket_rom_sha1(rom, n, hash);
  for (unsigned i = 0; i < sizeof(builds) / sizeof(*builds); i++)
    if (!strcmp(hash, builds[i].sha))
      return (int)i;
  return -1;
}
_Static_assert(sizeof tmItems / sizeof *tmItems <= PD_POCKET_SLOTS,
               "TM pocket capacity");
static unsigned be16(const unsigned char *p) { return p[0] * 256u + p[1]; }
// Gen2Reader.kt's well-formed pocket checks keep title-screen leftovers out.
static bool bag_pairs(struct pd_snapshot *s, pd_read_fn read, void *ctx,
                      uint32_t addr, int max, int pocket) {
  unsigned char raw[152];
  if (max > 75 || !read(ctx, addr, raw, (size_t)2 + 2 * max))
    return false;
  unsigned count = raw[0];
  if (count > (unsigned)max || raw[1 + 2 * count] != 255)
    return false;
  struct pd_pocket_items *p = &s->bag[pocket];
  for (unsigned i = 0; i < count; i++)
    if (raw[1 + 2 * i] > 0 && raw[1 + 2 * i] < 255) {
      if (p->count >= PD_POCKET_SLOTS)
        return false;
      p->items[p->count++] = (struct pd_item){raw[1 + 2 * i], raw[2 + 2 * i]};
    }
  return true;
}
static void read_bag(struct pd_snapshot *s, pd_read_fn read, void *ctx) {
  unsigned char keys[39], bits[(sizeof tmItems / sizeof *tmItems + 7) / 8],
      money[3];
  s->money = -1;
  if (!bag_pairs(s, read, ctx, PC_RAM_numItems, 75, PD_POCKET_ITEMS) ||
      !bag_pairs(s, read, ctx, PC_RAM_numMedicine, 37, PD_POCKET_ITEMS) ||
      !bag_pairs(s, read, ctx, PC_RAM_numBalls, 25, PD_POCKET_BALLS) ||
      !bag_pairs(s, read, ctx, PC_RAM_numBerries, 31, PD_POCKET_BERRIES) ||
      !read(ctx, PC_RAM_numKeyItems, keys, sizeof keys) ||
      !read(ctx, PC_RAM_tmsHms, bits, sizeof bits))
    goto invalid;
  int k = 0;
  while (k < 39 && keys[k])
    k++;
  if (k == 39)
    goto invalid;
  for (int i = 0; i < k; i++)
    s->bag[PD_POCKET_KEY].items[s->bag[PD_POCKET_KEY].count++] =
        (struct pd_item){0x200 + keys[i], 1};
  for (unsigned i = 0; i < sizeof tmItems / sizeof *tmItems; i++)
    if ((bits[i / 8] >> (i % 8)) & 1)
      s->bag[PD_POCKET_TMHM].items[s->bag[PD_POCKET_TMHM].count++] =
          (struct pd_item){tmItems[i], 1};
  if (read(ctx, PC_RAM_money, money, 3))
    s->money = (long)money[0] * 65536 + money[1] * 256 + money[2];
  s->bagOk = true;
  return;
invalid:
  memset(s->bag, 0, sizeof s->bag);
  s->bagOk = false;
}
static void read_sections(struct pd_snapshot *, pd_read_fn, void *);
bool pc_read(struct pd_snapshot *s, pd_read_fn read, void *ctx) {
  memset(s, 0, sizeof(*s));
  s->partyActive = -1;
  unsigned char count, raw[288];
  if (!read(ctx, PC_RAM_partyCount, &count, 1) || count > 6 ||
      !read(ctx, PC_RAM_partyMons, raw, sizeof raw))
    return false;
  s->partyCount = count;
  s->valid = true;
  for (int i = 0; i < count; i++) {
    unsigned char *p = raw + i * 48;
    int form = p[21], id = p[0] | ((form & 32) ? 256 : 0);
    for (unsigned k = 0; k < sizeof variants / sizeof *variants; k++)
      if (variants[k] == ((id << 8) | (form & 31))) {
        id = 292 + (int)k;
        break;
      }
    if (!p[0] || p[0] == 255 || id >= (int)(sizeof genders / sizeof *genders) ||
        !p[31] || p[31] > 100) {
      s->valid = false;
      s->partyCount = 0;
      return false;
    }
    struct pd_mon *m = &s->party[i];
    m->species = (form & 64) ? 255 : id;
    m->level = p[31];
    m->status = p[32];
    m->heldItem = p[1];
    m->experience = ((uint32_t)p[8] << 16) | ((uint32_t)p[9] << 8) | p[10];
    m->hp = be16(p + 34);
    m->maxHp = be16(p + 36);
    if (m->hp > m->maxHp || m->maxHp > 999) {
      s->valid = false;
      s->partyCount = 0;
      return false;
    }
    m->gender = (form & 64) || genders[id] == 255 ? PD_GENDER_NONE
                : genders[id] == 254              ? PD_GENDER_FEMALE
                : genders[id] == 0                ? PD_GENDER_MALE
                : (form & 128)                    ? PD_GENDER_FEMALE
                                                  : PD_GENDER_MALE;
    for (int j = 0; j < 4; j++) {
      m->moves[j] = p[j + 2];
      m->pp[j] = p[j + 22] & 63;
    }
  }
  read_bag(s, read, ctx);
  read_sections(s, read, ctx);
  return true;
}
static size_t off(unsigned a) {
  unsigned b = a >> 16, v = a & 65535;
  return b ? b * 0x4000 + (v & 0x3fff) : v;
}
int pc_lz(const unsigned char *rom, size_t size, size_t p, unsigned char *out,
          size_t capacity) {
  int used = 0;
#define BYTE(v)                                                                \
  do {                                                                         \
    if (p >= size)                                                             \
      return -1;                                                               \
    (v) = rom[p++];                                                            \
  } while (0)
  while (1) {
    int b;
    BYTE(b);
    if (b == 255)
      return used;
    int cmd = b >> 5, n = (b & 31) + 1;
    if (cmd == 7) {
      int v;
      BYTE(v);
      cmd = (b >> 2) & 7;
      n = ((b & 1) * 256 + v) + 1;
    }
    if (cmd > 6)
      return -1;
    if (cmd == 1)
      n++;
    if (cmd == 2)
      n += 2;
    if ((size_t)n > capacity - (size_t)used)
      return -1;
    int a = 0, c = 0, from = 0;
    if (cmd == 1 || cmd == 2)
      BYTE(a);
    if (cmd == 2)
      BYTE(c);
    if (cmd >= 4) {
      BYTE(a);
      if (a & 128)
        from = used - (a & 127) - 1;
      else {
        BYTE(c);
        from = a * 256 + c;
      }
    }
    for (int i = 0; i < n; i++) {
      int v = 0;
      if (cmd == 0)
        BYTE(v);
      else if (cmd == 1)
        v = a;
      else if (cmd == 2)
        v = (i & 1) ? c : a;
      else if (cmd >= 4) {
        int at = from + (cmd == 6 ? -i : i);
        if (at < 0 || at >= used)
          return -1;
        v = out[at];
        if (cmd == 5) {
          int r = 0;
          for (int k = 0; k < 8; k++)
            r |= ((v >> k) & 1) << (7 - k);
          v = r;
        }
      }
      out[used++] = (unsigned char)v;
    }
  }
#undef BYTE
}
static uint32_t rgb(const unsigned char *p) {
  unsigned v = p[0] + 256 * p[1];
  unsigned r = v & 31, g = (v >> 5) & 31, b = (v >> 10) & 31;
  return (((r << 3) | (r >> 2)) << 16) | (((g << 3) | (g >> 2)) << 8) |
         (b << 3) | (b >> 2);
}
static void icon_scaled(struct pd_canvas *c, int x, int y, int id,
                        const unsigned char *rom, size_t n, int build,
                        int scale) {
  if (!rom || build < 0 || build >= (int)(sizeof builds / sizeof *builds) ||
      id < 1)
    return;
  const unsigned *a = builds[build].at;
  int mini = id > 291 ? 291 + 56 + (id - 292) : id - 1;
  size_t e = off(a[1]) + 7 * mini, pa = off(a[2]) + 8 * id;
  if (e + 7 > n || pa + 4 > n)
    return;
  unsigned ptr = (rom[e] << 16) | rom[e + 1] | (rom[e + 2] << 8);
  unsigned char tiles[512];
  if (pc_lz(rom, n, off(ptr), tiles, sizeof tiles) < 128)
    return;
  uint32_t colors[4] = {0, rgb(rom + pa), rgb(rom + pa + 2), 0};
  for (int tile = 0; tile < 4; tile++)
    for (int yy = 0; yy < 8; yy++)
      for (int xx = 0; xx < 8; xx++) {
        int v = ((tiles[tile * 16 + yy * 2] >> (7 - xx)) & 1) |
                (((tiles[tile * 16 + yy * 2 + 1] >> (7 - xx)) & 1) << 1);
        if (v)
          pd_fill(c, x + (tile % 2 * 8 + xx) * scale,
                  y + (tile / 2 * 8 + yy) * scale, scale, scale, colors[v]);
      }
}
static void icon(struct pd_canvas *c, int x, int y, int id,
                 const unsigned char *rom, size_t n, int build) {
  icon_scaled(c, x, y, id, rom, n, build, 2);
}
// Gen2Art.holdIcons: first tile is Mail, second tile is the item marker.
static void held_icon(struct pd_canvas *c, int x, int y,
                      const unsigned char *rom, size_t n, int build) {
  if (!rom || build < 0 || build >= (int)(sizeof builds / sizeof *builds))
    return;
  size_t tile = off(builds[build].at[3]) + 16, pal = off(builds[build].at[4]);
  if (tile + 16 > n || pal + 8 > n)
    return;
  for (int yy = 0; yy < 8; yy++)
    for (int xx = 0; xx < 8; xx++) {
      int v = ((rom[tile + 2 * yy] >> (7 - xx)) & 1) |
              (((rom[tile + 2 * yy + 1] >> (7 - xx)) & 1) << 1);
      if (v)
        pd_fill(c, x + xx * 2, y + yy * 2, 2, 2, rgb(rom + pal + 2 * v));
    }
}
static const char *item_name(int id, int build) {
  if (id <= 0)
    return "None";
  if (id >= 0x300)
    return "Unknown item";
  bool faithful = build >= 0 && build < (int)(sizeof builds / sizeof *builds) &&
                  builds[build].faithful;
  const char *name =
      faithful && faithfulItems[id] ? faithfulItems[id] : itemNames[id];
  return name ? name : "Unknown item";
}
static void header(struct pd_canvas *c, int tab) {
  pd_fill(c, 0, 0, c->w, c->h, 0xffffff);
  pd_fill(c, 4, 4, c->w - 8, 26, 0x181818);
  const char *labels[] = {"PARTY", "BAG", "BATTLE", "DEX", "MAP", "GUIDE"};
  int start = tab >= 5 ? tab - 4 : 0;
  for (int i = 0; i < 5; i++) {
    if (i + start == tab)
      pd_fill(c, 5 + i * 100, 7, 98, 20, 0xffffff);
    pd_text(c, 4 + i * 100 + (100 - pd_text_width(labels[i + start])) / 2, 8,
            labels[i + start], i + start == tab ? 0x181818 : 0xffffff,
            PD_NO_SHADOW);
  }
}
static void txt(struct pd_canvas *c, int x, int y, const char *s) {
  pd_text(c, x, y, s, 0x181818, PD_NO_SHADOW);
}
static void pc_summary_draw(struct pd_canvas *, const struct pd_snapshot *, int,
                            int, int, int, const unsigned char *, size_t, int);
static void pc_move_draw(struct pd_canvas *, const struct pd_mon *, int,
                         const unsigned char *, size_t, int);
static void pc_party_draw(struct pd_canvas *c, const struct pd_snapshot *s,
                          int selected, bool summary, const unsigned char *rom,
                          size_t n, int build, int iconBob) {
  if (summary && s->valid && selected >= 0 && selected < s->partyCount) {
    pc_summary_draw(c, s, selected, 0, 0, 0, rom, n, build);
    return;
  }
  header(c, PC_PARTY);
  if (!s->valid) {
    txt(c, 20, 70, "Party data is not ready.");
    return;
  }
  /* PartyScreen.SlotGeometry, drawSlotFrame and drawHpBar at u=2,
   * stretched to the existing two-column controller grid. Polished's ball
   * palette is transparent, so no unrelated Pokeball artwork is added. */
  for (int i = 0; i < 6; i++) {
    int x = 8 + (i % 2) * 250, y = 40 + (i / 2) * 100;
    bool empty = i >= s->partyCount;
    unsigned paper = empty ? 0xf2f2f2 : i == selected ? 0xdcdcdc : 0xffffff;
    pd_round_rect(c, x + 16, y + 12, 224, 76, 6, empty ? 0xaaaaaa : 0x181818);
    pd_round_rect(c, x + 20, y + 16, 216, 68, 2, paper);
    if (empty)
      continue;
    const struct pd_mon *m = &s->party[i];
    char b[64];
    icon_scaled(c, x, y + 4 + (i == selected ? iconBob : 0), m->species, rom, n,
                build, 4);
    if (m->heldItem)
      held_icon(c, x + 32, y + 56, rom, n, build);
    const char *name = m->species == 255  ? "Egg"
                       : m->species < 512 ? names[m->species]
                                          : NULL;
    pd_text_fit(c, x + 76, y + 20, 156, name ? name : "Unknown", 0x181818,
                PD_NO_SHADOW);
    if (m->species == 255)
      continue;
    snprintf(b, sizeof b, "Lv%d", m->level);
    txt(c, x + 88, y + 34, b);
    pd_text_right(c, x + 220, y + 34,
                  m->gender == PD_GENDER_FEMALE ? "♀"
                  : m->gender == PD_GENDER_MALE ? "♂"
                                                : "",
                  0x181818, PD_NO_SHADOW);
    pocket_hp_bar(c, x + 78, y + 52, 154, m->hp, m->maxHp, &polishedHp);
    snprintf(b, sizeof b, "%d/%d", m->hp, m->maxHp);
    pd_text_right(c, x + 232, y + 68, b, 0x181818, PD_NO_SHADOW);
    txt(c, x + 76, y + 68, m->hp ? pd_status_label(m->status) : "FNT");
  }
  txt(c, 12, 346, "D-pad:Choose A:Summary");
  txt(c, 12, 366, "L/R:Section B/X:Play");
}

void pc_draw(struct pd_canvas *c, const struct pd_snapshot *s, int selected,
             bool summary, const unsigned char *rom, size_t n, int build) {
  pc_party_draw(c, s, selected, summary, rom, n, build, 0);
}
void pc_bag_draw(struct pd_canvas *c, const struct pd_snapshot *s, int pocket,
                 int selected, int build) {
  header(c, 1);
  static const char *labels[] = {"ITEMS", "BALLS", "TM/HM", "BERRY", "KEY"};
  if (pocket < 0 || pocket >= PD_POCKET_COUNT)
    pocket = 0;
  for (int i = 0; i < 5; i++) {
    int x = 8 + i * 100;
    pd_fill(c, x, 38, 96, 24, 0x181818);
    pd_fill(c, x + 1, 39, 94, 22, i == pocket ? 0xdcdcdc : 0xffffff);
    txt(c, x + 7, 41, labels[i]);
  }
  if (!s->bagOk) {
    txt(c, 20, 90, "Bag data is not ready.");
    return;
  }
  const struct pd_pocket_items *p = &s->bag[pocket];
  if (selected < 0)
    selected = 0;
  if (selected >= p->count)
    selected = p->count ? p->count - 1 : 0;
  int scroll = selected >= 10 ? selected - 9 : 0;
  if (!p->count)
    txt(c, 20, 90, "This pocket is empty.");
  for (int row = 0; row < 10 && scroll + row < p->count; row++) {
    int i = scroll + row, y = 72 + row * 24;
    if (i == selected) {
      pd_fill(c, 8, y, 496, 23, 0xdcdcdc);
      txt(c, 13, y + 2, ">");
    }
    pd_text_fit(c, 30, y + 2, 366, item_name(p->items[i].id, build), 0x181818,
                PD_NO_SHADOW);
    char qty[24];
    snprintf(qty, sizeof qty, "x%d", p->items[i].quantity);
    pd_text_right(c, 492, y + 2, qty, 0x181818, PD_NO_SHADOW);
  }
  char info[64];
  snprintf(info, sizeof info, "Money: %ld    %d/%d", s->money,
           p->count ? selected + 1 : 0, p->count);
  txt(c, 12, 322, info);
  txt(c, 12, 346, "D-pad:Browse A:Info");
  txt(c, 12, 366, "L/R:Section B/X:Play");
}

#define lz pc_lz
#include "polished_sections.inc"

#include "polished_parity.inc"
