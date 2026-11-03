// make test-gbdex: a Game Boy game's POKéDEX flags read from the app's captured WRAM (fixtures/<name>/wram.bin, wramx.bin).
// The recorded values are the app's: PolishedCrystalDecodeTest (caught {16, 155}, seen {16, 152, 155}); the rest must
// be self-consistent (owned counts as seen, every bit inside the game's Dex).
#include "../core/pd_game.c"
#include <stdio.h>
#include <stdlib.h>

static uint8_t wram[0x2000], wramx[0x8000];
static bool rd(void* c, uint32_t a, void* o, size_t n) {
    (void) c;
    if (a >= 0x10000) {
        uint32_t b = a >> 16, x = (a & 0xFFFF) - 0xD000;
        if (b > 7 || x + n > 0x1000) return false;
        memcpy(o, wramx + b * 0x1000 + x, n);
        return true;
    }
    if (a >= 0xC000 && a + n <= 0xE000) { memcpy(o, wram + a - 0xC000, n); return true; }
    return false;
}

static int load(const char* dir, const char* name, void* buf, size_t len) {
    char p[1024];
    snprintf(p, sizeof p, "%s/%s", dir, name);
    FILE* f = fopen(p, "rb");
    if (!f) return 0;
    size_t got = fread(buf, 1, len, f);
    fclose(f);
    return got > 0;
}

struct want { const char* fixture; const struct pd_config* cfg; int dexCount; const int* caught; const int* seen; };
static const int POLISHED_CAUGHT[] = { 16, 155, 0 }, POLISHED_SEEN[] = { 16, 152, 155, 0 };
#define W(f, C, n) { f, &PD_GB_CONFIG_##C, n, NULL, NULL }
static const struct want WANT[] = {
    W("red", RED, 151), W("blue", BLUE, 151), W("yellow", YELLOW, 151), W("purered", PURE_RED, 151),
    W("crystal", CRYSTAL, 251), W("gold", GOLD, 251), W("silver", GOLD, 251), W("crystal_clear", CRYSTAL_CLEAR, 251),
    { "polished_crystal", &PD_GB_CONFIG_POLISHED, 0x123, POLISHED_CAUGHT, POLISHED_SEEN },
};

static bool in(const int* l, int v) {
    for (; l && *l; l++) if (*l == v) return true;
    return false;
}

int main(int argc, char** argv) {
    int bad = 0, games = 0;
    if (argc < 2) return 2;
    for (size_t w = 0; w < sizeof WANT / sizeof WANT[0]; w++) {
        const struct want* t = &WANT[w];
        char dir[1024];
        snprintf(dir, sizeof dir, "%s/%s", argv[1], t->fixture);
        memset(wram, 0, sizeof wram);
        memset(wramx, 0, sizeof wramx);
        if (!load(dir, "wram.bin", wram, sizeof wram)) { printf("skip %s: no fixture\n", t->fixture); continue; }
        load(dir, "wramx.bin", wramx, sizeof wramx);
        struct pd_game g;
        memset(&g, 0, sizeof g);
        g.cfg = t->cfg;
        g.kind = PD_GAME_GB;
        static struct pd_snapshot s;
        memset(&s, 0, sizeof s);
        pd_snapshot_read(&s, &g, rd, NULL);
        games++;
        int seen = 0, caught = 0;
        for (int n = 1; n <= 0x123; n++) {
            bool sn = pd_dex_seen(&s, n), cg = pd_dex_caught(&s, n);
            if (cg && !sn) { printf("FAIL %s: No.%d owned but not seen\n", t->fixture, n); bad++; }
            if (n > t->dexCount && (sn || cg)) { printf("FAIL %s: No.%d past the Dex\n", t->fixture, n); bad++; }
            seen += sn;
            caught += cg;
            if (t->caught) {
                if (cg != in(t->caught, n)) { printf("FAIL %s: No.%d caught=%d\n", t->fixture, n, cg); bad++; }
                if (sn != in(t->seen, n)) { printf("FAIL %s: No.%d seen=%d\n", t->fixture, n, sn); bad++; }
            }
        }
        if (!s.dexOk || s.dexCount != t->dexCount) { printf("FAIL %s: dexOk=%d count=%d\n", t->fixture, s.dexOk, s.dexCount); bad++; }
        printf("  %s: seen %d, caught %d\n", t->fixture, seen, caught);
    }
    printf("%s (%d games)\n", bad || !games ? "FAILED" : "PASS gb dex", games);
    return bad != 0 || !games;
}
