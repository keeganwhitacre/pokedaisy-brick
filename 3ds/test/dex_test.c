// The dex save read per game (the app's readPokedexState): each fixture is decoded with its own static
// config and the seen / caught sets are those the app's Kotlin tests assert (HackPokedexTest and the
// *DecodeTest files). Usage: pd_dex_test <fixtures dir>
#include "../core/pd_game.c" // for the static PD_CONFIG_* tables
#include "../core/pd_snapshot.h"
#include <stdio.h>
#include <stdlib.h>

static uint8_t ram[0x40000], iram[0x8000];
static bool rd(void* c, uint32_t a, void* o, size_t n) {
    (void) c;
    if (a >= 0x02000000 && a < 0x02040000 && n <= 0x40000 - (a - 0x02000000)) { memcpy(o, ram + a - 0x02000000, n); return true; }
    if (a >= 0x03000000 && a < 0x03008000 && n <= 0x8000 - (a - 0x03000000)) { memcpy(o, iram + a - 0x03000000, n); return true; }
    return false;
}

struct want { const char* fixture; const struct pd_config* cfg; const char* name; int seen[8], caught[8]; };
#define W(fx, C, ...) { fx, &PD_CONFIG_##C, #C, __VA_ARGS__ }
static const struct want WANT[] = {
    W("dreamstone", DREAMSTONE, { 25, 152, 255, 501, 906 }, { 25, 152, 255, 501, 906 }),
    W("wishes", WISHES, { 6, 25, 94, 155, 448 }, { 6, 25, 94, 155, 448 }),
    W("rowe_battle", ROWE, { 265, 884 }, { 884 }),
    W("imperium", IMPERIUM, { 4 }, { 4 }),
    W("glazed", GLAZED, { 174, 322 }, { 322 }),
    W("unbound_fr", UNBOUND_FR, { 225, 246, 361, 374, 451, 582, 686, 774 }, { 225, 246, 361 }),
    W("orange_islands", ORANGE_ISLANDS, { 16, 25, 150 }, { 25, 150 }),
    W("odyssey_dex", ODYSSEY, { 137, 263, 285, 303, 311, 312, 316, 352 }, { 311, 312 }),
    W("radical_red_dex", RADICAL_RED, { 4, 7 }, { 4 }),
    W("amethyst_dex", AMETHYST, { 182, 340 }, { 340 }),
    W("gaia_dex", GAIA, { 390 }, { 390 }),
    W("celia_dex", CELIA, { 2 }, { 2 }),
    W("heart_and_soul_dex", HEART_AND_SOUL, { 16, 155 }, { 155 }),
    W("lazarus_dex", LAZARUS, { 653 }, { 653 }),
    W("soulgold", SOULGOLD, { 16, 19, 155, 163, 183, 263 }, { 155 }),
    W("soulgold_v12", SOULGOLD_V1_2, { 16, 19, 155, 163, 183, 263 }, { 155 }),
    W("emerald_seaglass_dex", SEAGLASS, { 4, 12 }, { 4 }),
    W("tmt2_dex", TMT2, { 397 }, { 397 }),
};

static int check(const struct pd_snapshot* s, bool caught, const int* want, const char* what) {
    int bad = 0, n = 0;
    for (int i = 0; i < 8 && want[i]; i++) n++;
    for (int k = 1; k <= PD_DEX_BYTES * 8; k++) {
        bool has = caught ? pd_dex_caught(s, k) : pd_dex_seen(s, k), expect = false;
        for (int i = 0; i < n; i++) expect |= want[i] == k;
        if (has != expect) { printf("FAIL %s: No. %d is %d, want %d\n", what, k, has, expect); bad++; }
    }
    return bad;
}

int main(int argc, char** argv) {
    int bad = 0;
    if (argc < 2) return 2;
    for (size_t w = 0; w < sizeof WANT / sizeof WANT[0]; w++) {
        char p[1024]; FILE* f;
        memset(ram, 0, sizeof ram); memset(iram, 0, sizeof iram);
        snprintf(p, sizeof p, "%s/%s/ewram.bin", argv[1], WANT[w].fixture);
        f = fopen(p, "rb"); if (!f) { printf("FAIL no %s\n", p); bad++; continue; }
        if (fread(ram, 1, sizeof ram, f) == 0) bad++;
        fclose(f);
        snprintf(p, sizeof p, "%s/%s/iwram.bin", argv[1], WANT[w].fixture);
        f = fopen(p, "rb"); if (f) { if (fread(iram, 1, sizeof iram, f) == 0) bad++; fclose(f); }
        struct pd_game g; memset(&g, 0, sizeof g); g.cfg = WANT[w].cfg; g.kind = PD_GAME_EMERALD;
        static struct pd_snapshot s; pd_snapshot_read(&s, &g, rd, NULL);
        char what[96]; snprintf(what, sizeof what, "%s/%s", WANT[w].fixture, WANT[w].name);
        if (!s.dexOk) { printf("FAIL %s: no dex\n", what); bad++; continue; }
        bad += check(&s, false, WANT[w].seen, what) + check(&s, true, WANT[w].caught, what);
    }
    printf("%s (%zu games)\n", bad ? "FAILED" : "PASS dex", sizeof WANT / sizeof WANT[0]);
    return bad != 0;
}
