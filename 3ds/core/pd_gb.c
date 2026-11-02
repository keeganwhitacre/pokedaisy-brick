#include "pd_gb.h"

#include <string.h>

#include "pd_game.h"
#include "pd_tables.h"

// Gen 1 / Gen 2 structs are plaintext and big-endian (Gen1Reader.kt, Gen2Reader.kt).
#define G1_MON 44
#define G2_MON 48
#define G1_BATTLE 29
#define G2_BATTLE 32
#define POLISHED_BATTLE 35
#define NAME_LEN 11
#define G2_EGG 0xFD
#define G2_TMS 57

extern const uint16_t pd_gb_internal_to_dex_yellow[], pd_gb_internal_to_dex_pure_rgb[], pd_gb_gender_crystal[],
    pd_gb_gender_prism[], pd_gb_gender_polished[], pd_gb_tm_crystal[], pd_gb_tm_prism[], pd_gb_tm_polished[];
extern const int pd_gb_internal_to_dex_yellow_count, pd_gb_internal_to_dex_pure_rgb_count, pd_gb_gender_crystal_count,
    pd_gb_gender_prism_count, pd_gb_gender_polished_count, pd_gb_tm_crystal_count, pd_gb_tm_prism_count,
    pd_gb_tm_polished_count;

#define POLISHED_NUM_SPECIES 0x123

const struct pd_gb_config pd_gb_yellow = {
    .gen = 1, .partyCount = 0xD162, .partyMons = 0xD16A, .numItems = 0xD31C, .money = 0xD346, .battleMode = 0xD056,
    .battleMon = 0xD013, .enemyMon = 0xCFE4, .otPartyCount = 0xD89B, .otPartyMons = 0xD8A3, .curOTMon = 0xCFE7,
    .internalToDex = pd_gb_internal_to_dex_yellow, .bagSlots = 20,
};
const struct pd_gb_config pd_gb_red = {
    .gen = 1, .partyCount = 0xD163, .partyMons = 0xD16B, .numItems = 0xD31D, .money = 0xD347, .battleMode = 0xD057,
    .battleMon = 0xD014, .enemyMon = 0xCFE5, .otPartyCount = 0xD89C, .otPartyMons = 0xD8A4, .curOTMon = 0xCFE8,
    .internalToDex = pd_gb_internal_to_dex_yellow, .bagSlots = 20,
};
const struct pd_gb_config pd_gb_pure_red = {
    .gen = 1, .partyCount = PD_WRAMX(1, 0xD16B), .partyMons = PD_WRAMX(1, 0xD173), .numItems = PD_WRAMX(1, 0xD542),
    .money = PD_WRAMX(1, 0xD34F), .battleMode = PD_WRAMX(1, 0xD057), .battleMon = PD_WRAMX(1, 0xD014),
    .enemyMon = 0xCFE3, .otPartyCount = PD_WRAMX(1, 0xD8A4), .otPartyMons = PD_WRAMX(1, 0xD8AC), .curOTMon = 0xCFE6,
    .internalToDex = pd_gb_internal_to_dex_pure_rgb, .bagSlots = 30,
};
const struct pd_gb_config pd_gb_crystal = {
    .gen = 2, .partyCount = PD_WRAMX(1, 0xDCD7), .partyMons = PD_WRAMX(1, 0xDCDF), .numItems = PD_WRAMX(1, 0xD892),
    .numKeyItems = PD_WRAMX(1, 0xD8BC), .numBalls = PD_WRAMX(1, 0xD8D7), .tmsHms = PD_WRAMX(1, 0xD859),
    .money = PD_WRAMX(1, 0xD84E), .battleMode = PD_WRAMX(1, 0xD22D), .battleMon = 0xC62C,
    .enemyMon = PD_WRAMX(1, 0xD206), .otPartyCount = PD_WRAMX(1, 0xD280), .otPartyMons = PD_WRAMX(1, 0xD288),
    .curOTMon = 0xC663, .genderRatios = pd_gb_gender_crystal, .tmItems = pd_gb_tm_crystal, .speciesCount = 251,
    .itemsMax = 20, .ballsMax = 12, .keyItemsMax = 25,
};
const struct pd_gb_config pd_gb_gold = {
    .gen = 2, .partyCount = PD_WRAMX(1, 0xDA22), .partyMons = PD_WRAMX(1, 0xDA2A), .numItems = PD_WRAMX(1, 0xD5B7),
    .numKeyItems = PD_WRAMX(1, 0xD5E1), .numBalls = PD_WRAMX(1, 0xD5FC), .tmsHms = PD_WRAMX(1, 0xD57E),
    .money = PD_WRAMX(1, 0xD573), .battleMode = PD_WRAMX(1, 0xD116), .battleMon = 0xCB0C,
    .enemyMon = PD_WRAMX(1, 0xD0EF), .otPartyCount = PD_WRAMX(1, 0xDD55), .otPartyMons = PD_WRAMX(1, 0xDD5D),
    .curOTMon = 0xCB41, .genderRatios = pd_gb_gender_crystal, .tmItems = pd_gb_tm_crystal, .speciesCount = 251,
    .itemsMax = 20, .ballsMax = 12, .keyItemsMax = 25,
};
// Prism: Crystal's RAM but the bag, and its own 254 species ids.
const struct pd_gb_config pd_gb_prism = {
    .gen = 2, .partyCount = PD_WRAMX(1, 0xDCD7), .partyMons = PD_WRAMX(1, 0xDCDF), .numItems = PD_WRAMX(1, 0xD866),
    .numKeyItems = PD_WRAMX(1, 0xD8B8), .numBalls = PD_WRAMX(1, 0xD8EC), .tmsHms = PD_WRAMX(1, 0xD859),
    .money = PD_WRAMX(1, 0xD84E), .battleMode = PD_WRAMX(1, 0xD22D), .battleMon = 0xC62C,
    .enemyMon = PD_WRAMX(1, 0xD206), .otPartyCount = PD_WRAMX(1, 0xD280), .otPartyMons = PD_WRAMX(1, 0xD288),
    .curOTMon = 0xC663, .genderRatios = pd_gb_gender_prism, .tmItems = pd_gb_tm_prism, .speciesCount = 254,
    .itemsMax = 40, .ballsMax = 25, .keyItemsMax = 50, .tmBits = true,
};
const struct pd_gb_config pd_gb_polished = {
    .gen = 2, .partyCount = PD_WRAMX(1, 0xDCCE), .partyMons = PD_WRAMX(1, 0xDCD6), .numItems = PD_WRAMX(1, 0xD827),
    .numKeyItems = PD_WRAMX(1, 0xD800), .numBalls = PD_WRAMX(1, 0xD90B), .tmsHms = PD_WRAMX(1, 0xD7F5),
    .money = PD_WRAMX(1, 0xD7E5), .battleMode = PD_WRAMX(1, 0xD233), .battleMon = 0xC4A5,
    .enemyMon = PD_WRAMX(1, 0xD209), .otPartyCount = PD_WRAMX(1, 0xD283), .otPartyMons = PD_WRAMX(1, 0xD28B),
    .curOTMon = 0xC4DD, .genderRatios = pd_gb_gender_polished, .tmItems = pd_gb_tm_polished,
    .speciesCount = POLISHED_NUM_SPECIES, .itemsMax = 75, .ballsMax = 25, .keyItemsMax = 38, .tmBits = true,
    .polished = true, .numMedicine = PD_WRAMX(1, 0xD8BF), .numBerries = PD_WRAMX(1, 0xD93F), .medicineMax = 37,
    .berriesMax = 31,
};

static int be16(const uint8_t* b) {
    return (b[0] << 8) | b[1];
}

// The cart's text (constants/charmap.asm) for the letters, digits and signs the companion prints; Polished Crystal ends
// names with 0x53 / 0x52 too.
static void gb_name(const uint8_t* src, bool polished, char* out, size_t outLen) {
    size_t n = 0;
    for (int i = 0; i < NAME_LEN && n + 4 < outLen; i++) {
        int c = src[i];
        if (c == 0x00 || c == 0x50 || (polished && (c == 0x52 || c == 0x53))) break;
        const char* s = NULL;
        char one[2] = {0, 0};
        if (c >= 0x80 && c <= 0x99) one[0] = (char) ('A' + c - 0x80);
        else if (c >= 0xA0 && c <= 0xB9) one[0] = (char) ('a' + c - 0xA0);
        else if (c >= 0xF6 && c <= 0xFF) one[0] = (char) ('0' + c - 0xF6);
        else switch (c) {
            case 0x7F: one[0] = ' '; break;
            case 0xE0: one[0] = '\''; break;
            case 0xE3: one[0] = '-'; break;
            case 0xE6: one[0] = '?'; break;
            case 0xE7: one[0] = '!'; break;
            case 0xE8: case 0xF2: one[0] = '.'; break;
            case 0xF3: one[0] = '/'; break;
            case 0xF4: one[0] = ','; break;
            case 0x9A: one[0] = '('; break;
            case 0x9B: one[0] = ')'; break;
            case 0x9C: one[0] = ':'; break;
            case 0xEF: s = "♂"; break;
            case 0xF5: s = "♀"; break;
            case 0xBA: s = "é"; break;
            default: break;
        }
        if (!s && one[0]) s = one;
        if (!s) continue;
        size_t l = strlen(s);
        memcpy(out + n, s, l);
        n += l;
    }
    out[n] = 0;
    // Trailing spaces.
    while (n && out[n - 1] == ' ') out[--n] = 0;
}

static bool rd(pd_read_fn read, void* ctx, uint32_t a, void* out, size_t n) {
    return read(ctx, a, out, n);
}

// Hidden Power from Gen 2's DVs (retail Gen 3's type numbering: Fighting 1 ... Dark 17, no 9).
static void gen2_hidden_power(struct pd_mon_stats* st, int atk, int def, int spe, int spc) {
    int k = (atk & 3) * 4 + (def & 3); // 0..15: Fighting ... Dark
    st->hiddenPowerType = k < 8 ? k + 1 : k + 2;
    int v = ((atk >> 3) << 3) | ((def >> 3) << 2) | ((spe >> 3) << 1) | (spc >> 3);
    st->hiddenPowerPower = (5 * v + (spc & 3)) / 2 + 31;
    st->hasHiddenPower = true;
}

static void gb_stats(struct pd_mon* m, const uint8_t* raw, const struct pd_gb_config* g) {
    int atk, def, spe, spc, base;
    const uint8_t* dv;
    if (g->gen == 1) {
        dv = raw + 27;
        base = 34;
    } else {
        if (g->polished) return;
        dv = raw + 21;
        base = 36;
    }
    atk = dv[0] >> 4;
    def = dv[0] & 15;
    spe = dv[1] >> 4;
    spc = dv[1] & 15;
    int hp = ((atk & 1) << 3) | ((def & 1) << 2) | ((spe & 1) << 1) | (spc & 1);
    struct pd_mon_stats* st = &m->stats;
    int ivs[6] = {hp, atk, def, spe, spc, spc};
    for (int i = 0; i < 6; i++) st->ivs[i] = (uint8_t) ivs[i];
    st->values[0] = be16(raw + base);
    st->values[1] = be16(raw + base + 2);
    st->values[2] = be16(raw + base + 4);
    st->values[3] = be16(raw + base + 6);
    st->values[4] = be16(raw + base + 8);
    st->values[5] = g->gen == 1 ? st->values[4] : be16(raw + base + 10);
    st->valid = st->hasIvs = true;
    if (g->gen == 2) gen2_hidden_power(st, atk, def, spe, spc);
}

static int polished_species(int species, int form) {
    int s9 = species | ((form & 0x20) ? 0x100 : 0);
    int key = (s9 << 8) | (form & 0x1F);
    for (int k = 0; k < pd_gb_polished_variants_count; k++)
        if ((int) pd_gb_polished_variants[k] == key) return POLISHED_NUM_SPECIES + 1 + k;
    return s9;
}

static int gen2_gender(const struct pd_gb_config* g, int species, const uint8_t* dv, bool formBit, bool polished) {
    if (species < 0 || species >= g->genderCount) return PD_GENDER_NONE;
    int r = g->genderRatios[species];
    if (r == 255) return PD_GENDER_NONE;
    if (r == 0) return PD_GENDER_MALE;
    if (r == 254) return PD_GENDER_FEMALE;
    if (polished) return formBit ? PD_GENDER_FEMALE : PD_GENDER_MALE;
    return r < ((dv[0] & 0xF0) | (dv[1] >> 4)) ? PD_GENDER_MALE : PD_GENDER_FEMALE;
}

static bool decode_mon(const struct pd_gb_config* g, const uint8_t* raw, bool egg, struct pd_mon* m) {
    memset(m, 0, sizeof *m);
    if (g->gen == 1) {
        int internal = raw[0];
        int dex = internal < g->internalCount ? g->internalToDex[internal] : 0;
        int level = raw[33];
        if (!dex || level < 1 || level > 100) return false;
        m->species = dex;
        m->level = level;
        m->hp = be16(raw + 1);
        m->maxHp = be16(raw + 34);
        m->status = raw[4];
        for (int i = 0; i < 4; i++) {
            m->moves[i] = raw[8 + i];
            m->pp[i] = raw[29 + i] & 0x3F;
        }
        m->experience = ((uint32_t) raw[14] << 16) | (raw[15] << 8) | raw[16];
        m->hasExperience = true;
        m->gender = PD_GENDER_NONE;
        m->personality = ((uint32_t) raw[12] << 24) | (raw[13] << 16) | (raw[27] << 8) | (raw[28] ^ (uint32_t) dex);
        gb_stats(m, raw, g);
        return true;
    }
    int species = raw[0];
    if (g->polished) {
        if (species == 0 || species == 0xFF) return false;
        int form = raw[21];
        egg = (form & 0x40) != 0;
        int id = polished_species(species, form);
        if (id < 1 || id >= g->genderCount) return false;
        species = id;
        m->gender = egg ? PD_GENDER_NONE : gen2_gender(g, id, raw + 17, (form & 0x80) != 0, true);
        for (int i = 0; i < 4; i++) m->pp[i] = raw[22 + i] & 0x3F;
        m->personality = ((uint32_t) raw[6] << 24) | (raw[7] << 16) | (raw[17] << 8) | raw[18];
    } else {
        if (species < 1 || species > g->speciesCount) return false;
        m->gender = egg ? PD_GENDER_NONE : gen2_gender(g, species, raw + 21, false, false);
        for (int i = 0; i < 4; i++) m->pp[i] = raw[23 + i] & 0x3F;
        m->personality = ((uint32_t) raw[6] << 24) | (raw[7] << 16) | (raw[21] << 8) | raw[22];
    }
    int level = raw[31];
    if (level < 1 || level > 100) return false;
    m->species = egg ? PD_SPECIES_EGG : species;
    m->level = level;
    m->hp = be16(raw + 34);
    m->maxHp = be16(raw + 36);
    m->status = raw[32];
    for (int i = 0; i < 4; i++) m->moves[i] = raw[2 + i];
    m->experience = ((uint32_t) raw[8] << 16) | (raw[9] << 8) | raw[10];
    m->hasExperience = true;
    m->heldItem = raw[1];
    m->hasHeldItem = true;
    if (!egg) gb_stats(m, raw, g);
    return true;
}

static void decode_battle(const struct pd_gb_config* g, const uint8_t* raw, struct pd_battle_mon* b) {
    memset(b, 0, sizeof *b);
    int species, level, t1, t2, hp, maxHp, status, moves, pp;
    if (g->gen == 1) {
        int internal = raw[0];
        species = internal < g->internalCount ? g->internalToDex[internal] : 0;
        level = raw[14];
        t1 = raw[5];
        t2 = raw[6];
        hp = be16(raw + 1);
        maxHp = be16(raw + 15);
        status = raw[4];
        moves = 8;
        pp = 25;
    } else if (g->polished) {
        if (raw[0] == 0 || raw[0] == 0xFF) return;
        species = polished_species(raw[0], raw[10]);
        level = raw[16];
        t1 = raw[33];
        t2 = raw[34];
        hp = be16(raw + 19);
        maxHp = be16(raw + 21);
        status = raw[17];
        moves = 2;
        pp = 11;
        b->heldItem = raw[1];
    } else {
        species = raw[0];
        if (species < 1 || species > g->speciesCount) return;
        level = raw[13];
        t1 = raw[30];
        t2 = raw[31];
        hp = be16(raw + 16);
        maxHp = be16(raw + 18);
        status = raw[14];
        moves = 2;
        pp = 8;
        b->heldItem = raw[1];
    }
    if (!species || level < 1 || level > 100) return;
    b->species = species;
    b->level = level;
    b->type1 = t1;
    b->type2 = t2;
    b->hp = hp;
    b->maxHp = maxHp;
    b->status = (uint32_t) status;
    for (int i = 0; i < 4; i++) {
        b->moves[i] = raw[moves + i];
        b->pp[i] = raw[pp + i] & 0x3F;
    }
}

static void add_item(struct pd_snapshot* s, int pocket, int id, int qty) {
    struct pd_pocket_items* p = &s->bag[pocket];
    if (p->count >= PD_POCKET_SLOTS) return;
    p->items[p->count].id = (uint16_t) id;
    p->items[p->count].quantity = (uint16_t) qty;
    p->count++;
}

// A list of {item, quantity} pairs after a count, ending in 0xFF; false when it isn't well formed (WRAM before a save).
static bool read_pairs(pd_read_fn read, void* ctx, uint32_t addr, int max, int pocket, struct pd_snapshot* s) {
    uint8_t raw[2 + 2 * 80];
    if (max > 80) max = 80;
    if (!rd(read, ctx, addr, raw, (size_t) (2 + 2 * max))) return false;
    int n = raw[0];
    if (n > max || raw[1 + 2 * n] != 0xFF) return false;
    for (int i = 0; i < n; i++) add_item(s, pocket, raw[1 + 2 * i], raw[2 + 2 * i]);
    return true;
}

static bool read_gen2_bag(const struct pd_gb_config* g, struct pd_snapshot* s, pd_read_fn read, void* ctx) {
    if (!read_pairs(read, ctx, g->numItems, g->itemsMax, PD_POCKET_ITEMS, s)) return false;
    if (!read_pairs(read, ctx, g->numBalls, g->ballsMax, PD_POCKET_BALLS, s)) return false;
    if (g->polished) {
        if (!read_pairs(read, ctx, g->numMedicine, g->medicineMax, PD_POCKET_ITEMS, s)) return false;
        if (!read_pairs(read, ctx, g->numBerries, g->berriesMax, PD_POCKET_BERRIES, s)) return false;
        uint8_t key[40];
        if (!rd(read, ctx, g->numKeyItems, key, (size_t) g->keyItemsMax + 1)) return false;
        int n = 0;
        while (n <= g->keyItemsMax && key[n]) n++;
        if (n > g->keyItemsMax) return false;
        for (int i = 0; i < n; i++) add_item(s, PD_POCKET_KEY, 0x200 + key[i], 1);
    } else {
        uint8_t key[2 + 50];
        if (!rd(read, ctx, g->numKeyItems, key, (size_t) (2 + g->keyItemsMax))) return false;
        int n = key[0];
        if (n > g->keyItemsMax || key[1 + n] != 0xFF) return false;
        for (int i = 0; i < n; i++) add_item(s, PD_POCKET_KEY, key[1 + i], 1);
    }
    if (g->tmBits) {
        uint8_t bits[16];
        int bytes = (g->tmCount + 7) / 8;
        if (bytes > 16 || !rd(read, ctx, g->tmsHms, bits, (size_t) bytes)) return false;
        for (int i = 0; i < g->tmCount; i++)
            if ((bits[i / 8] >> (i % 8)) & 1) add_item(s, PD_POCKET_TMHM, g->tmItems[i], 1);
    } else {
        uint8_t tm[G2_TMS];
        if (!rd(read, ctx, g->tmsHms, tm, sizeof tm)) return false;
        for (int i = 0; i < G2_TMS; i++)
            if (tm[i] > 99) return false;
        for (int i = 0; i < G2_TMS && i < g->tmCount; i++)
            if (tm[i]) add_item(s, PD_POCKET_TMHM, g->tmItems[i], tm[i]);
    }
    return true;
}

// The generated tables' lengths, which a static initializer can't name.
static void fill_counts(struct pd_gb_config* c) {
    if (c->internalToDex == pd_gb_internal_to_dex_yellow) c->internalCount = pd_gb_internal_to_dex_yellow_count;
    if (c->internalToDex == pd_gb_internal_to_dex_pure_rgb) c->internalCount = pd_gb_internal_to_dex_pure_rgb_count;
    if (c->genderRatios == pd_gb_gender_crystal) c->genderCount = pd_gb_gender_crystal_count;
    if (c->genderRatios == pd_gb_gender_prism) c->genderCount = pd_gb_gender_prism_count;
    if (c->genderRatios == pd_gb_gender_polished) c->genderCount = pd_gb_gender_polished_count;
    if (c->tmItems == pd_gb_tm_crystal) c->tmCount = pd_gb_tm_crystal_count;
    if (c->tmItems == pd_gb_tm_prism) c->tmCount = pd_gb_tm_prism_count;
    if (c->tmItems == pd_gb_tm_polished) c->tmCount = pd_gb_tm_polished_count;
}

void pd_gb_snapshot_read(struct pd_snapshot* s, const struct pd_config* cfg, pd_read_fn read, void* ctx) {
    struct pd_gb_config copy = *cfg->gb;
    fill_counts(&copy);
    const struct pd_gb_config* g = &copy;
    s->valid = true;
    s->partyActive = -1;
    s->money = -1;
    s->foeActive = s->foeNext = -1;
    int monSize = g->gen == 1 ? G1_MON : G2_MON;

    // --- party: the count, then the species list (Gen 2 ends it in 0xFF; Polished Crystal keeps none) ---
    uint8_t head[1 + PD_PARTY_SIZE + 1] = {0};
    if (!rd(read, ctx, g->partyCount, head, sizeof head)) return;
    int count = head[0];
    if (count > PD_PARTY_SIZE || (g->gen == 2 && !g->polished && count != PD_PARTY_SIZE && head[1 + count] != 0xFF) ||
        (g->gen == 1 && head[1 + count] != 0xFF))
        count = 0;
    uint8_t raw[PD_PARTY_SIZE * G2_MON + PD_PARTY_SIZE * NAME_LEN * 2];
    size_t span = (size_t) PD_PARTY_SIZE * monSize + 2 * (size_t) PD_PARTY_SIZE * NAME_LEN;
    if (count && rd(read, ctx, g->partyMons, raw, span)) {
        const uint8_t* nick = raw + PD_PARTY_SIZE * monSize + PD_PARTY_SIZE * NAME_LEN;
        for (int i = 0; i < count; i++) {
            struct pd_mon* m = &s->party[s->partyCount];
            if (!decode_mon(g, raw + i * monSize, g->gen == 2 && !g->polished && head[1 + i] == G2_EGG, m)) continue;
            gb_name(nick + i * NAME_LEN, g->polished, m->nickname, sizeof m->nickname);
            s->partyCount++;
        }
    }

    // --- bag ---
    if (g->gen == 1) {
        uint8_t bag[1 + 2 * 30];
        int slots = g->bagSlots > 30 ? 30 : g->bagSlots;
        if (rd(read, ctx, g->numItems, bag, (size_t) (1 + 2 * slots))) {
            int n = bag[0] > slots ? slots : bag[0];
            s->bagOk = true;
            for (int i = 0; i < n && bag[1 + 2 * i] != 0xFF; i++) add_item(s, PD_POCKET_ITEMS, bag[1 + 2 * i], bag[2 + 2 * i]);
        }
    } else {
        s->bagOk = read_gen2_bag(g, s, read, ctx);
        if (!s->bagOk) memset(s->bag, 0, sizeof s->bag);
    }

    // --- money ---
    uint8_t m3[3];
    if (rd(read, ctx, g->money, m3, 3)) {
        if (g->gen == 1) {
            long v = 0;
            for (int i = 0; i < 3; i++) v = v * 100 + (m3[i] >> 4) * 10 + (m3[i] & 15);
            s->money = v;
        } else {
            s->money = ((long) m3[0] << 16) | (m3[1] << 8) | m3[2];
        }
    }

    // --- battle ---
    uint8_t mode = 0;
    rd(read, ctx, g->battleMode, &mode, 1);
    s->inBattle = mode == 1 || mode == 2;
    if (!s->inBattle) return;
    s->isTrainer = mode == 2;
    uint8_t bm[G2_BATTLE + 8];
    int bsize = g->gen == 1 ? G1_BATTLE : g->polished ? POLISHED_BATTLE : G2_BATTLE;
    if (rd(read, ctx, g->battleMon, bm, (size_t) bsize)) decode_battle(g, bm, &s->battlers[PD_POS_PLAYER_LEFT]);
    if (rd(read, ctx, g->enemyMon, bm, (size_t) bsize)) decode_battle(g, bm, &s->battlers[PD_POS_OPPONENT_LEFT]);
    if (mode == 2) {
        uint8_t n = 0;
        rd(read, ctx, g->otPartyCount, &n, 1);
        if (n > PD_PARTY_SIZE) n = PD_PARTY_SIZE;
        uint8_t ot[PD_PARTY_SIZE * G2_MON];
        if (n && rd(read, ctx, g->otPartyMons, ot, (size_t) PD_PARTY_SIZE * monSize)) {
            for (int i = 0; i < n; i++)
                if (decode_mon(g, ot + i * monSize, false, &s->foes[s->foeCount])) s->foeCount++;
            uint8_t cur = 0;
            rd(read, ctx, g->curOTMon, &cur, 1);
            if (cur < s->foeCount) s->foeActive = cur;
        }
    }
}
