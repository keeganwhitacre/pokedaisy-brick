// make test-roms ROMS=<dir>: every ROM under <dir> (one level of folders) is recognised: a config, or a named game the
// companion says it can't read. "Unknown game" and "ROM hack" titles fail. Folders starting with _ or . are skipped.
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/pd_game.h"
#include "../core/pd_snapshot.h"
#include "../core/pd_tables.h"

static uint8_t rom[33 << 20];

static int check(const char* path, const char* name, int* ok, int* unsupported) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    size_t n = fread(rom, 1, sizeof rom, f);
    fclose(f);
    if (n < 0x150) return 0;
    struct pd_game g;
    pd_game_detect(&g, rom, n);
    g.rom = rom;
    g.romSize = n;
    pd_game_identify(&g);
    bool generic = !g.title || strstr(g.title, "Unknown") || strstr(g.title, "ROM hack") || strstr(g.title, "Game Boy ROM");
    if (generic) { printf("FAIL %s: not recognised (%s: %s)\n", name, g.title ? g.title : "?", g.unsupported ? g.unsupported : ""); return 1; }
    // A GBA game the companion reads names its map sections (WHERE IS, HERE, MAP): its own table, its ROM's
    // sMapNames or its engine's retail names - the app's activeMapSecData.
    if (g.cfg && g.kind != PD_GAME_GB) {
        int named = 0;
        const char* first = NULL;
        for (int m = 0; m < 0x200; m++) {
            const char* n = pd_mapsec_name(&g, m);
            if (n[0]) { named++; if (!first) first = n; }
        }
        const struct pd_text* t = g.cfg->text;
        // its items' descriptions come from its ROM (the app's RomItemText): item 4's (a POKé BALL in every game so far)
        const char* desc = pd_item_description(&g, 4);
        if (getenv("PD_ROMS_NAMES")) printf("  %s: item 4 '%s'\n", name, desc);
        // (a build guessed onto a neighbour's config may keep its table elsewhere: the app has none for it either)
        if (!desc[0] && !strstr(g.title, "unverified")) { printf("FAIL %s: no item description\n", name); return 1; }
        if (getenv("PD_ROMS_NAMES"))
            printf("  %s: %d place names (%s), first %s\n", name, named,
                   t && t->mapsecsRom ? "ROM" : t && t->mapsecs ? "own table" : "retail", first ? first : "-");
        if (named < 20) { printf("FAIL %s: %d place names\n", name, named); return 1; }
    }
    if (g.cfg) (*ok)++;
    else { (*unsupported)++; printf("  %s: %s (%s)\n", name, g.title, g.unsupported ? g.unsupported : "?"); }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    int bad = 0, ok = 0, unsupported = 0;
    DIR* d = opendir(argv[1]);
    if (!d) return 2;
    struct dirent* e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.' || e->d_name[0] == '_') continue;
        char sub[1024];
        snprintf(sub, sizeof sub, "%s/%s", argv[1], e->d_name);
        DIR* s = opendir(sub);
        if (!s) continue;
        struct dirent* r;
        while ((r = readdir(s))) {
            const char* x = strrchr(r->d_name, '.');
            if (r->d_name[0] == '.' || !x || (strcmp(x, ".gba") && strcmp(x, ".gb") && strcmp(x, ".gbc"))) continue;
            char path[2048];
            snprintf(path, sizeof path, "%s/%s", sub, r->d_name);
            bad += check(path, r->d_name, &ok, &unsupported);
        }
        closedir(s);
    }
    closedir(d);
    printf("%s (%d with a config, %d recognised but unsupported)\n", bad ? "FAILED" : "PASS roms", ok, unsupported);
    return bad != 0;
}
