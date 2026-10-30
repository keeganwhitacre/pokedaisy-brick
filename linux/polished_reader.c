/* GPL-3.0; reader recovered from pocket/polished.c, adapted to current snapshots. */
#include "polished_reader.h"
#include "pd_tables.h"
#include "polished_reader_data.h"
#include <string.h>
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

bool pc_snapshot_read(struct pd_snapshot *s, pd_read_fn read, void *ctx) {
  memset(s, 0, sizeof(*s));

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
    m->species = (form & 64) ? PD_SPECIES_EGG : id;
    m->level = p[31];
    m->status = p[32];


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

  return true;
}
