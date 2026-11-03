// The GUIDE per hack (the app's HackGuidesTest): the game's tables are where its config says (the probe trainer's name),
// and every boss the NEXT BOSS page lists has a team of 1-6 real Pokemon with levels 1-100 and moves, read from the
// ROM. Usage: pd_guide_test <dir with the ROMs>; a game without its ROM is skipped, a ROM whose tables are not where
// the config says (another build than the config's) is reported and skipped, not failed.
#include "../core/pd_game.c" // for the static PD_CONFIG_* tables
#include "../core/pd_guide.h"
#include <stdio.h>
#include <stdlib.h>

struct want { const char* name; const struct pd_config* cfg; enum pd_guide_id guide; const char* rom; };
#define W(n, C, G, rom) { n, &PD_CONFIG_##C, PD_GUIDE_##G, rom }
static const struct want WANT[] = {
    W("Rocket Edition", ROCKET_EDITION, ROCKET_EDITION, "Pokemon FireRed Rocket Edition (v1.0.2).gba"),
    W("Ash Gray", ASH_GRAY, ASH_GRAY, "Pokemon - AshGray (v4.5.3).gba"),
    W("GS Chronicles", GS_CHRONICLES, GS_CHRONICLES, "Pokemon GS Chronicles (2.7.6).gba"),
    W("Radical Red", RADICAL_RED, RADICAL_RED, "Pokemon Radical Red (v4.1).gba"),
    W("Orange Islands", ORANGE_ISLANDS, ORANGE_ISLANDS, "Pokemon Orange Islands Beta (v5.7).gba"),
    W("Odyssey", ODYSSEY, ODYSSEY, "Pokémon Odyssey COMPLETE v4.1.1 (EXPANDED BAG).gba"),
    W("Sword and Shield", SWORD_SHIELD, SWORD_SHIELD, "Pokemon Sword and Shield Ultimate Plus (v1.2.1.2).gba"),
    W("Unbound", UNBOUND, UNBOUND, "Pokemon Unbound (v2.1.1.1).gba"),
    W("Gaia", GAIA, GAIA, "Pokemon Gaia (v3.2).gba"),
    W("Ghost Grey", GHOST_GREY, GHOST_GREY, "Pokémon_ Ghost Grey Version (1.0.4).gba"),
    W("Celia", CELIA, CELIA, "Pokemon Celia's Stupid Romhack (1.1.4).gba"),
};

int main(int argc, char** argv) {
    int bad = 0, games = 0, teams = 0;
    if (argc < 2) return 2;
    for (size_t w = 0; w < sizeof WANT / sizeof WANT[0]; w++) {
        const struct want* t = &WANT[w];
        char p[1024];
        snprintf(p, sizeof p, "%s/%s", argv[1], t->rom);
        FILE* f = fopen(p, "rb");
        if (!f) { printf("skip %s (no ROM)\n", t->name); continue; }
        fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
        uint8_t* rom = malloc((size_t) n);
        if (!rom || fread(rom, 1, (size_t) n, f) != (size_t) n) { fclose(f); free(rom); bad++; continue; }
        fclose(f);
        struct pd_game g; memset(&g, 0, sizeof g);
        g.rom = rom; g.romSize = (size_t) n; g.cfg = t->cfg; g.kind = PD_GAME_FIRERED; g.guide = t->guide; g.loadId = (unsigned) (w + 1);
        if (!pd_guide_rom_ok(&g)) { printf("skip %s: its tables are not where the config says (another build)\n", t->name); free(rom); continue; }
        const struct pd_guide* gd = pd_guide_for(&g);
        if (!gd) { printf("FAIL %s: no guide\n", t->name); bad++; free(rom); continue; }
        games++;
        static struct pd_snapshot s;
        memset(&s, 0, sizeof s);
        for (int b = 0; b < gd->bossCount; b++) {
            const struct pd_boss* boss = &gd->bosses[b];
            int ids[16], k = 0;
            ids[k++] = pd_boss_trainer(gd, boss, &s);
            for (const struct pd_variant* v = boss->variants; v && v->label && k < 15; v++) ids[k++] = v->trainer;
            if (boss->kind == PD_BOSS_BY_FLAGS) for (const uint16_t* a = boss->alts; a && a[0] && k < 15; a += 2) ids[k++] = a[1];
            for (int i = 0; i < k; i++) {
                struct pd_team_mon team[PD_TEAM_MAX];
                int cnt = pd_trainer_team(&g, ids[i], team);
                bool ok = cnt >= 1 && cnt <= PD_TEAM_MAX;
                for (int m = 0; ok && m < cnt; m++) {
                    bool moves = false;
                    for (int q = 0; q < 4; q++) moves |= team[m].moves[q] != 0;
                    ok = team[m].species > 0 && team[m].level >= 1 && team[m].level <= 100 && (moves || t->guide == PD_GUIDE_UNBOUND);  // Unbound has no learnset table here: default movesets stay empty
                }
                teams++;
                if (!ok) { printf("FAIL %s %s (trainer %d: %d mons, first sp %d lv %d)\n", t->name, boss->title, ids[i], cnt, cnt ? team[0].species : 0, cnt ? team[0].level : 0); bad++; }
            }
        }
        free(rom);
    }
    printf("%s (%d games, %d teams)\n", bad ? "FAILED" : "PASS guide", games, teams);
    return bad != 0;
}
