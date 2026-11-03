// make test-gbguide: Red, Blue and Yellow's NEXT BOSS (GuideGen1.kt's facts, Gen1GuideTest.kt): the teams as each game
// loads them, the champion's team by the rival's pick, the badge flags of the app's fixtures, the League done for good.
#include "../core/pd_game.c"
#include "../core/pd_guide.h"
#include <stdio.h>
#include <stdlib.h>

static uint8_t wram[0x2000];
static bool rd(void* c, uint32_t a, void* o, size_t n) {
    (void) c;
    if (a >= 0xC000 && a + n <= 0xE000) { memcpy(o, wram + a - 0xC000, n); return true; }
    return false;
}

static int bad;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL " __VA_ARGS__); printf("\n"); bad++; } } while (0)

static bool done(const struct pd_snapshot* s, const struct pd_boss* b) {
    bool d = pd_flag(s, b->doneFlag);
    for (const uint16_t* f = b->doneAny; f && *f && !d; f++) d = pd_flag(s, *f);
    return d;
}

static const struct pd_boss* next_boss(const struct pd_guide* gd, const struct pd_snapshot* s) {
    for (int i = 0; i < gd->bossCount; i++) if (!done(s, &gd->bosses[i])) return &gd->bosses[i];
    return NULL;
}

static int team_of(const struct pd_game* g, const struct pd_guide* gd, const struct pd_boss* b, const struct pd_snapshot* s,
                   struct pd_team_mon* t) {
    return pd_trainer_team(g, pd_boss_trainer(gd, b, s), t);
}

static void game(const char* fixtures, const char* fixture, const struct pd_config* cfg, const struct pd_guide* gd,
                 int brock1, int brock1lv, int brock2lv, int teamVarSquirtle, int teamVarCharmander, int lastSquirtle, int lastCharmander, int guardBit, bool leagueOpen) {
    char p[1024];
    snprintf(p, sizeof p, "%s/%s/wram.bin", fixtures, fixture);
    memset(wram, 0, sizeof wram);
    FILE* f = fopen(p, "rb");
    if (!f) { printf("skip %s: no fixture\n", fixture); return; }
    fread(wram, 1, sizeof wram, f);
    fclose(f);
    struct pd_game g;
    memset(&g, 0, sizeof g);
    g.cfg = cfg;
    g.kind = PD_GAME_GB;
    static struct pd_snapshot save, fresh;
    memset(&save, 0, sizeof save);
    pd_snapshot_read(&save, &g, rd, NULL);
    CHECK(save.flagsOk, "%s: guide flags not read", fixture);
    for (int b = 0; b < 8; b++) CHECK(pd_flag(&save, 368 * 8 + b), "%s: badge %d", fixture, b);
    CHECK(gd->bossCount == 13, "%s: %d bosses", fixture, gd->bossCount);
    CHECK(!strcmp(gd->bosses[12].title, "CHAMPION, YOUR RIVAL"), "%s: last boss %s", fixture, gd->bosses[12].title);

    // A fresh game: BROCK's team.
    memset(&fresh, 0, sizeof fresh);
    fresh.flagsOk = true;
    struct pd_team_mon t[PD_TEAM_MAX];
    const struct pd_boss* nb = next_boss(gd, &fresh);
    int n = team_of(&g, gd, nb, &fresh, t);
    CHECK(n == 2 && t[0].species == brock1 && t[0].level == brock1lv && t[1].species == 95 && t[1].level == brock2lv,
          "%s: BROCK's team (%d mons, %d lv%d, %d lv%d)", fixture, n, t[0].species, t[0].level, t[1].species, t[1].level);
    for (int b = 0; b < gd->bossCount; b++) {
        const struct pd_boss* bs = &gd->bosses[b];
        for (int v = 0; v < 4; v++) {
            int id = bs->variants ? (bs->variants[v].label ? (int) bs->variants[v].trainer : -1) : (v ? -1 : (int) bs->a);
            if (id < 0) break;
            int c = pd_trainer_team(&g, id, t);
            bool ok = c >= 1 && c <= 6;
            for (int i = 0; i < c; i++) ok &= t[i].level >= 2 && t[i].level <= 100 && (t[i].moves[0] || t[i].moves[1] || t[i].moves[2] || t[i].moves[3]);
            CHECK(ok, "%s: %s team %d reads %d mons", fixture, bs->title, id, c);
            if (!bs->variants) break;
        }
    }

    // The fixture save has all 8 badges: the League is next.
    nb = next_boss(gd, &save);
    // (The Yellow save has already beaten the League: nothing is left.)
    CHECK(leagueOpen ? nb && !strcmp(nb->title, "ELITE FOUR LORELEI") : !nb, "%s: next boss with 8 badges is %s", fixture, nb ? nb->title : "none");

    // The champion's team follows the rival's pick; before the byte is set, every team.
    const struct pd_boss* champ = &gd->bosses[12];
    CHECK(champ->kind == PD_BOSS_TEAMVAR && champ->variants, "%s: champion has no variants", fixture);
    fresh.vars[0] = (uint16_t) teamVarSquirtle;
    n = team_of(&g, gd, champ, &fresh, t);
    CHECK(n && t[n - 1].species == lastSquirtle, "%s: SQUIRTLE team ends %d", fixture, n ? t[n - 1].species : 0);
    fresh.vars[0] = (uint16_t) teamVarCharmander;
    n = team_of(&g, gd, champ, &fresh, t);
    CHECK(n && t[n - 1].species == lastCharmander, "%s: CHARMANDER team ends %d", fixture, n ? t[n - 1].species : 0);
    int teams = 0;
    for (const struct pd_variant* v = champ->variants; v->label; v++) teams++;
    CHECK(teams == 3, "%s: %d champion teams", fixture, teams);

    // The Hall of Fame clears the League's flags but hides the CERULEAN CAVE guard for good (missable 9).
    memset(&fresh, 0, sizeof fresh);
    fresh.flagsOk = true;
    fresh.flags[368] = 0xFF;
    fresh.flags[320 + 1] = (uint8_t) (1 << guardBit);  // TOGGLE_CERULEAN_CAVE_GUY: missable 9 (Yellow), 8 (Red / Blue)
    CHECK(!next_boss(gd, &fresh), "%s: a boss is left after the Hall of Fame", fixture);
    printf("  %s ok\n", fixture);
}

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    game(argv[1], "yellow", &PD_GB_CONFIG_YELLOW, &pd_guide_gb_yellow, 74, 10, 12, 1, 3, 135, 134, 1, false);
    game(argv[1], "red", &PD_GB_CONFIG_RED, &pd_guide_gb_red, 74, 12, 14, 177, 176, 9, 6, 0, true);
    game(argv[1], "blue", &PD_GB_CONFIG_BLUE, &pd_guide_gb_blue, 74, 12, 14, 177, 176, 9, 6, 0, true);
    printf("%s\n", bad ? "FAILED" : "PASS gb guide");
    return bad != 0;
}
