#include "pd_game.h"

#include <stdio.h>
#include <string.h>

#include "pd_sha1.h"

// The configs come from gamedata/ (gamedata/targets/3ds.toml picks the fields
// this port reads), generated with the app's: `make gamedata`.
const struct pd_party_layout pd_party_layout_vanilla = {
    .size = 100, .status = 0x50, .level = 0x54, .hp = 0x56, .maxHp = 0x58, .flags = 0x13, .eggMask = 0x04,
    .plainBox = 0x20, .plainSpeciesMask = 0xFFFF, .plainItemMask = 0xFFFF, .hpBits = 16, .statusMask = 0xFFFFFFFFu,
    .hasStats = true, .natureModifier = true, .nicknameOff = 0x08, .nicknameLen = 10,
};
// struct BattlePokemon layouts (Gen3Mon.kt): retail's / CFRU's, pokeemerald-expansion's and the hacks' variants of it.
const struct pd_battle_layout pd_battle_layout_vanilla = {
    .size = 88, .species = 0, .moves = 12, .type1 = 33, .type2 = 34, .pp = 36, .hp = 40, .level = 42,
    .maxHp = 44, .status = 76, .speciesMask = 0xFFFF,
};
const struct pd_battle_layout pd_battle_layout_expansion = {
    .size = 96, .species = 0, .moves = 12, .type1 = 34, .type2 = 35, .pp = 37, .hp = 42, .level = 44,
    .maxHp = 46, .status = 80, .speciesMask = 0xFFFF,
};
const struct pd_battle_layout pd_battle_layout_tmt2 = {
    .size = 96, .species = 0, .moves = 12, .type1 = 34, .type2 = 35, .pp = 38, .hp = 42, .level = 44,
    .maxHp = 46, .status = 80, .speciesMask = 0xFFFF,
};
const struct pd_battle_layout pd_battle_layout_team_rocket = {
    .size = 92, .species = 0, .moves = 12, .type1 = 34, .type2 = 35, .pp = 37, .hp = 42, .level = 44,
    .maxHp = 46, .status = 80, .speciesMask = 0xFFFF,
};
const struct pd_battle_layout pd_battle_layout_wishes = {
    .size = 136, .species = 0, .moves = 12, .type1 = 34, .type2 = 35, .pp = 37, .hp = 42, .level = 44,
    .maxHp = 46, .status = 80, .speciesMask = 0xFFFF,
};
const struct pd_battle_layout pd_battle_layout_rowe = {
    .size = 96, .species = 0, .moves = 12, .type1 = 34, .type2 = 35, .pp = 37, .hp = 42, .level = 44,
    .maxHp = 46, .status = 80, .speciesMask = 0x7FF,
};
const struct pd_battle_layout pd_battle_layout_soulgold = {
    .size = 152, .species = 0, .moves = 12, .type1 = 34, .type2 = 35, .pp = 37, .hp = 42, .level = 44,
    .maxHp = 46, .status = 84, .speciesMask = 0xFFFF,
};
const struct pd_battle_layout pd_battle_layout_elite_redux = {
    .size = 104, .species = 20, .moves = 32, .type1 = 68, .type2 = 69, .pp = 71, .hp = 54, .level = 75,
    .maxHp = 56, .status = 8, .speciesMask = 0xFFFF,
};
// Pokemon Quetzal: a 0x68-byte plaintext struct (expansion's, bit-packed HP and status), egg flag in the IV word.
const struct pd_party_layout pd_party_layout_quetzal = {
    .size = 0x68, .status = 0x24, .level = 0x58, .hp = 0x23, .maxHp = 0x5A, .flags = 0x13, .eggMask = 0x04,
    .plainBox = 0x28, .plainSpeciesMask = 0xFFFF, .plainItemMask = 0xFFFF, .plainExp = true, .hpBits = 10,
    .statusShift = 2, .statusMask = 0xFFF, .eggInIvWord = true, .hasStats = true, .natureModifier = true,
    .nicknameOff = 0x08, .nicknameLen = 10,
};
// SoulGold: 96 bytes, a 12-letter nickname; no stats (the offsets here don't add up) and its species / item are narrower.
const struct pd_party_layout pd_party_layout_soulgold = {
    .size = 96, .status = 0x4C, .level = 0x50, .hp = 0x52, .maxHp = 0x54, .flags = 0x15, .eggMask = 0x04,
    .plainBox = 0x20, .plainSpeciesMask = 0x07FF, .plainItemMask = 0x03FF, .hpBits = 16, .statusMask = 0xFFFFFFFFu,
    .hasStats = false, .natureModifier = true, .nicknameOff = 0x08, .nicknameLen = 12,
};
// Team Rocket Edition (Dragonsden): retail's struct, the flags in the language byte, no Mints.
const struct pd_party_layout pd_party_layout_team_rocket = {
    .size = 100, .status = 0x50, .level = 0x54, .hp = 0x56, .maxHp = 0x58, .flags = 0x12, .eggMask = 0x20,
    .plainBox = 0x20, .plainSpeciesMask = 0xFFFF, .plainItemMask = 0xFFFF, .hpBits = 16, .statusMask = 0xFFFFFFFFu,
    .hasStats = true, .natureModifier = false, .nicknameOff = 0x08, .nicknameLen = 10,
};
// R.O.W.E. 2.x: 0x4C bytes, plaintext and bit-packed (Gen3Mon.kt decodeRoweMon).
const struct pd_party_layout pd_party_layout_rowe = {
    .size = 0x4C, .status = 0x38, .level = 0x3C, .hp = 0x3E, .maxHp = 0x40, .flags = 0x27, .eggMask = 0x80,
    .plainBox = 0x20, .plainSpeciesMask = 0xFFFF, .plainItemMask = 0xFFFF, .hpBits = 16, .statusMask = 0xFFFFFFFFu,
    .hasStats = true, .nicknameOff = 0x08, .nicknameLen = 12, .rowe = true,
};
#include "pd_configs_gen.h"

const uint8_t* pd_rom_at(const struct pd_game* g, uint32_t addr, size_t len) {
    if (!g->rom || addr < 0x08000000) return NULL;
    size_t off = addr - 0x08000000;
    if (off > g->romSize || len > g->romSize - off) return NULL;
    return g->rom + off;
}

unsigned pd_rom_id(const struct pd_game* g) {
    return g->rom ? g->loadId : 0;
}

bool pd_rom_read(const struct pd_game* g, uint32_t addr, void* out, size_t len) {
    const uint8_t* p = pd_rom_at(g, addr, len);
    if (!p) return false;
    memcpy(out, p, len);
    return true;
}

void pd_game_detect(struct pd_game* g, const uint8_t* header, size_t romSize) {
    static unsigned loads;
    memset(g, 0, sizeof(*g));
    g->loadId = ++loads ? loads : ++loads;
    memcpy(g->code, header + 0xAC, 4);
    g->code[4] = 0;
    g->revision = header[0xBC];

    if (!strcmp(g->code, "BPRE") || !strcmp(g->code, "BPGE")) {
        g->kind = PD_GAME_FIRERED;
        g->title = g->code[2] == 'R' ? "Pokémon FireRed" : "Pokémon LeafGreen";
        bool rev1 = g->revision == 1;
        g->guide = g->code[2] == 'R' ? PD_GUIDE_FIRERED : PD_GUIDE_LEAFGREEN;
        if (g->code[2] == 'R') g->cfg = rev1 ? &PD_CONFIG_FIRERED_REV1 : &PD_CONFIG_FIRERED_REV0;
        else g->cfg = rev1 ? &PD_CONFIG_LEAFGREEN_REV1 : &PD_CONFIG_LEAFGREEN_REV0;
    } else if (!strcmp(g->code, "AXVE") || !strcmp(g->code, "AXPE")) {
        // Ruby / Sapphire (USA), read like Emerald (Poller.otherRetailConfig): rev 0 has its own addresses.
        bool ruby = g->code[2] == 'V';
        g->kind = PD_GAME_EMERALD;
        g->title = ruby ? "Pokémon Ruby" : "Pokémon Sapphire";
        g->guide = ruby ? PD_GUIDE_RUBY : PD_GUIDE_SAPPHIRE;
        if (g->revision == 0) g->cfg = ruby ? &PD_CONFIG_RUBY_REV0 : &PD_CONFIG_SAPPHIRE_REV0;
        else g->cfg = ruby ? &PD_CONFIG_RUBY : &PD_CONFIG_SAPPHIRE;
        if (g->revision > 2) {
            g->cfg = NULL;
            g->unsupported = "This revision isn't supported yet";
        } else if (romSize > 0x1000000) {
            g->cfg = NULL;
            g->title = ruby ? "Ruby ROM hack" : "Sapphire ROM hack";
            g->unsupported = "Not a version the companion knows";
        }
        return;
    } else if (!strcmp(g->code, "BPEE")) {
        g->kind = PD_GAME_EMERALD;
        g->title = "Pokémon Emerald";
        g->guide = PD_GUIDE_EMERALD;
        g->cfg = &PD_CONFIG_EMERALD;
    } else {
        g->title = "Unknown game";
        g->unsupported = "Not a game the 3DS companion knows yet";
        return;
    }
    if (g->revision > 1) {
        g->cfg = NULL;
        g->unsupported = "This revision isn't supported yet";
    } else if (romSize > 0x1000000) {
        // A >16 MB FireRed / Emerald is a ROM hack: the app tells them apart
        // by SHA1 (Poller.detect) - not ported yet.
        g->cfg = NULL;
        g->title = g->kind == PD_GAME_FIRERED ? "FireRed ROM hack" : "Emerald ROM hack";
        g->unsupported = "ROM hacks aren't supported on 3DS yet";
    }
}

// The retail carts (USA / Europe) detect's configs are for: GameTitles.RETAIL.
static const char* const RETAIL_SHA1S[] = {
    "41cb23d8dccc8ebd7c649cd8fbb58eeace6e2fdc", // FireRed rev 0
    "dd5945db9b930750cb39d00c84da8571feebf417", // FireRed rev 1
    "574fa542ffebb14be69902d1d36f1ec0a4afd71e", // LeafGreen rev 0
    "7862c67bdecbe21d1d69ce082ce34327e1c6ed5e", // LeafGreen rev 1
    "f3ae088181bf583e55daf962a92bb46f4f1d07b7", // Emerald
};

// A collection's dump of a retail cart (the app's RETAIL_DUMPS, RetailDumpsGen.kt): bytes [0, end) with the
// ranges zeroed hash to sha1 - the end's padding trimmed, the save code patched, the wait states changed.
struct retail_dump {
    const char* code;
    int rev;
    uint32_t end;
    uint32_t ranges[6][2]; // [from, to)
    const char* sha1;
};
static const struct retail_dump RETAIL_DUMPS[] = {
    {"BPRE", 0, 0xEB0B1E,
     {{0xC4, 0xCA}, {0x460, 0x464}, {0x1DE824, 0x1DF128}, {0x415BFF, 0x415C8F}, {0x6FBEB8, 0x6FC03C}, {0x71A23C, 0xD00000}},
     "7c5ed3b3ce796ef2ef05878b31836b090dc7b7b5"},
    {"BPRE", 1, 0xEB0B1E,
     {{0xC4, 0xCA}, {0x470, 0x474}, {0x1DE894, 0x1DF198}, {0x415C6F, 0x415CFF}, {0x6FBF18, 0x6FC09C}, {0x71A29C, 0xD00000}},
     "4b1c9554caf64c58055e7e7202fa5bf99f86bea1"},
    {"BPEE", 0, 0xE3CF51,
     {{0xC4, 0xCA}, {0x470, 0x474}, {0x2E1860, 0x2E2A2C}, {0x5E83B6, 0x5E84D3}, {0x9A3064, 0x9A3234}},
     "09d7e7cbb3cfbf842c7663c63d3c8fb8eb6423f8"},
};

// The hacks (and other versions) the companion reads, by SHA-1: Poller.detect's *_SHA1 cases, one per config the
// 3DS / Linux target generates (gamedata/targets/3ds.toml).
struct known_rom {
    const char* sha1;
    const struct pd_config* cfg;
    enum pd_game_kind kind;
    const char* title;
    enum pd_guide_id guide;
};
static const struct known_rom KNOWN_ROMS[] = {
    {"cbd71af65ee3dcd53280a6a12014bfb11e8f626a", &PD_CONFIG_EMERALD_LEGACY, PD_GAME_EMERALD, "Pokémon Emerald Legacy",
     PD_GUIDE_EMERALD_LEGACY},
    {"a7c4e34fafb53f2d5283eb6d43dad285c0dd40a8", &PD_CONFIG_EMERALD_KAIZO, PD_GAME_EMERALD, "Pokémon Emerald Kaizo",
     PD_GUIDE_NONE},
    {"e10105d8544469c6a11ca2cf510289981df3c3b0", &PD_CONFIG_GLAZED, PD_GAME_EMERALD, "Pokémon Glazed",
     PD_GUIDE_NONE},
    {"3cbd5a2e72ce60cfae1e0c28d83e01f855c51537", &PD_CONFIG_CELIA, PD_GAME_FIRERED, "Pokémon Celia's Stupid Romhack",
     PD_GUIDE_NONE},
    {"8bac897de515f88e6ce92d0712ce2c018a394b37", &PD_CONFIG_ORANGE_ISLANDS, PD_GAME_FIRERED, "Pokémon Orange Islands",
     PD_GUIDE_NONE},
    {"d2e3800e69e44c9649c3d9a15dc1cd12c1d90501", &PD_CONFIG_ORANGE_ISLANDS, PD_GAME_FIRERED, "Pokémon Orange Islands",
     PD_GUIDE_NONE},
    {"3b6c51d8fb6b513bdd01b7bda3ed8965bc985d58", &PD_CONFIG_ULTRA_VIOLET, PD_GAME_FIRERED, "Pokémon Ultra Violet",
     PD_GUIDE_NONE},
    {"033d05374ae34eb84fae62c693ca8f3da23dd1bd", &PD_CONFIG_ROCKET_EDITION, PD_GAME_FIRERED, "Pokémon FireRed Rocket Edition",
     PD_GUIDE_NONE},
    {"1ee1557db1e4c2e0e94c6306efcc40506bc40cc9", &PD_CONFIG_ASH_GRAY, PD_GAME_FIRERED, "Pokémon Ash Gray",
     PD_GUIDE_NONE},
    {"92ed82145c013069472153391c7aab1cfbf37b75", &PD_CONFIG_ASH_GRAY, PD_GAME_FIRERED, "Pokémon Ash Gray Fork Bug Fix",
     PD_GUIDE_NONE},
    {"d5b1e77975fcda831e0e9a7b527906bf3f40ecd0", &PD_CONFIG_GAIA, PD_GAME_FIRERED, "Pokémon Gaia",
     PD_GUIDE_NONE},
    {"8745ddbdbfadf6abaf66de4e9055923b62eb4668", &PD_CONFIG_ODYSSEY, PD_GAME_FIRERED, "Pokémon Odyssey",
     PD_GUIDE_NONE},
    {"00e70c0384a5f1698588034201fd5b849d3542e2", &PD_CONFIG_AMETHYST, PD_GAME_FIRERED, "Pokémon Amethyst",
     PD_GUIDE_NONE},
    {"91291aade04b4b111cd03ae7b6e2ff460e1edd8a", &PD_CONFIG_AMETHYST_V1_4_1, PD_GAME_FIRERED, "Pokémon Amethyst",
     PD_GUIDE_NONE},
    {"25f8840ecb58e584edf18ffde35f845e94f3f17c", &PD_CONFIG_GS_CHRONICLES, PD_GAME_FIRERED, "Pokémon GS Chronicles",
     PD_GUIDE_NONE},
    {"20dfaea1d5317cf95fe0b493208edce7d50b865f", &PD_CONFIG_GHOST_GREY, PD_GAME_FIRERED, "Pokémon Ghost Grey",
     PD_GUIDE_NONE},
    {"74337272566a74625d6dedefe47d2d153eb48ec1", &PD_CONFIG_GHOST_GREY, PD_GAME_FIRERED, "Pokémon Ghost Grey",
     PD_GUIDE_NONE},
    {"0dafc0116b74406a2df491e26ccb40dd362fb0b7", &PD_CONFIG_GHOST_GREY, PD_GAME_FIRERED, "Pokémon Ghost Grey",
     PD_GUIDE_NONE},
    {"afe4c8e0f9c7186860fcea5f25d8318f5d7929b9", &PD_CONFIG_SWORD_SHIELD, PD_GAME_FIRERED, "Pokémon Sword and Shield Ultimate Plus",
     PD_GUIDE_NONE},
    {"daa2f89934d2c0447d5271dabeba86bfba04e1fd", &PD_CONFIG_SWORD_SHIELD, PD_GAME_FIRERED, "Pokémon Sword and Shield Ultimate Plus",
     PD_GUIDE_NONE},
    {"4858cc3d7b335bb5eed553861d7a4a17bad9ce2c", &PD_CONFIG_SWORD_SHIELD, PD_GAME_FIRERED, "Pokémon Sword and Shield Ultimate Plus",
     PD_GUIDE_NONE},
    {"932323d1fa3732d8673d1319fa86ed4df41087b9", &PD_CONFIG_SWORD_SHIELD, PD_GAME_FIRERED, "Pokémon Sword and Shield Ultimate Plus",
     PD_GUIDE_NONE},
    {"964f951a0fdaf209e4ea1344883ef0d557bb3a80", &PD_CONFIG_RADICAL_RED, PD_GAME_FIRERED, "Pokémon Radical Red",
     PD_GUIDE_NONE},
    {"b4776b82a4c7915d0fadeaa27e013523f99dfd94", &PD_CONFIG_UNBOUND, PD_GAME_FIRERED, "Pokémon Unbound",
     PD_GUIDE_NONE},
    {"0ce2a880aa097f1dce4e1db8ee513d0e82d15859", &PD_CONFIG_UNBOUND_FR, PD_GAME_FIRERED, "Pokémon Unbound",
     PD_GUIDE_NONE},
    {"1d20091c4d936f5eb122db8780554dd0829ffb63", &PD_CONFIG_IMPERIUM, PD_GAME_EMERALD, "Pokémon Emerald Imperium",
     PD_GUIDE_NONE},
    {"3fa8e61ec1727cc51540c54677623f736a446e1b", &PD_CONFIG_TMT2, PD_GAME_EMERALD, "Pokémon Too Many Types 2",
     PD_GUIDE_NONE},
    {"8a6940ae3fb9a27577ab24833d7c928eb4d8db87", &PD_CONFIG_TMT2, PD_GAME_EMERALD, "Pokémon Too Many Types 2 (unverified)",
     PD_GUIDE_NONE},
    {"be5033598e91d0e80fa93e551406f02a7876d7c2", &PD_CONFIG_RECHARGED_YELLOW_V1_8_0, PD_GAME_FIRERED, "Pokémon Recharged Yellow",
     PD_GUIDE_NONE},
    {"ac30d1ed80e75104a3fb92f71e2b9a51c26e80c8", &PD_CONFIG_RECHARGED_YELLOW_V1_9_0, PD_GAME_FIRERED, "Pokémon Recharged Yellow",
     PD_GUIDE_NONE},
    {"7d9fae6cf31c07bc7fa5cf7afe137ad5385cefb1", &PD_CONFIG_RECHARGED_YELLOW_V1_9_4, PD_GAME_FIRERED, "Pokémon Recharged Yellow",
     PD_GUIDE_NONE},
    {"ed60654d7525ba005bfc8f799bf0ecba2ef08a2e", &PD_CONFIG_RECHARGED_YELLOW_V1_9_7, PD_GAME_FIRERED, "Pokémon Recharged Yellow",
     PD_GUIDE_NONE},
    {"b9f4d332d30fc88c379f9e037f9eae3b2755ead4", &PD_CONFIG_SEAGLASS, PD_GAME_EMERALD, "Pokémon Emerald Seaglass",
     PD_GUIDE_NONE},
    {"7dcdc7e280bc4631487e13dd37e6e0cea04adea6", &PD_CONFIG_LAZARUS, PD_GAME_EMERALD, "Pokémon Lazarus",
     PD_GUIDE_NONE},
    {"60182b934d2e8eb3eef226a9bbc25e5df6becd4f", &PD_CONFIG_WISHES, PD_GAME_EMERALD, "Pokémon Wishes of Tomorrow",
     PD_GUIDE_NONE},
    {"06e79d12afa578db97c23c1f7ba771fe2f8fe905", &PD_CONFIG_MARIOMON_V1_5_2, PD_GAME_EMERALD, "Super Mariomon",
     PD_GUIDE_NONE},
    {"831fe0f97301f17b24a9cd4a64ef0520835c4dde", &PD_CONFIG_MARIOMON_V1_5_1, PD_GAME_EMERALD, "Super Mariomon",
     PD_GUIDE_NONE},
    {"c2ee8cc6b0a80bde0f4fb0d4a2e1fb984bccec77", &PD_CONFIG_MARIOMON_V1_5_1, PD_GAME_EMERALD, "Super Mariomon (unverified)",
     PD_GUIDE_NONE},
    {"043512c2497871c018df606fb90ffbd4c81430f4", &PD_CONFIG_MARIOMON_V1_3, PD_GAME_EMERALD, "Super Mariomon",
     PD_GUIDE_NONE},
    {"08d1c275c0ba4d559b33427292702d61ad49fb5c", &PD_CONFIG_MARIOMON_V1_2_2, PD_GAME_EMERALD, "Super Mariomon",
     PD_GUIDE_NONE},
    {"43dd208b23eade69aa3384301e934ecafbf6c2b3", &PD_CONFIG_MARIOMON_V1_2_1, PD_GAME_EMERALD, "Super Mariomon",
     PD_GUIDE_NONE},
    {"81bd0f4bfa1c04ab2c6faab1bddd10e8a390ea77", &PD_CONFIG_ROWE, PD_GAME_EMERALD, "Pokémon R.O.W.E.",
     PD_GUIDE_NONE},
    {"40780347d0b4df6bf491eaf12603e5422caee677", &PD_CONFIG_TEAM_ROCKET, PD_GAME_EMERALD, "Pokémon Team Rocket Edition",
     PD_GUIDE_NONE},
    {"20e91aecb3243eea7cd610659846f37062f5d709", &PD_CONFIG_DREAMSTONE, PD_GAME_EMERALD, "Pokémon Dreamstone Mysteries",
     PD_GUIDE_NONE},
    {"ea5d369cc8a31cbf1cfacb7c9470ea670f08957b", &PD_CONFIG_SOULGOLD, PD_GAME_EMERALD, "Pokémon SoulGold",
     PD_GUIDE_NONE},
    {"805d880ee229fb6dc3ce03d7b03baf48f0d759d0", &PD_CONFIG_SOULGOLD_V1_2, PD_GAME_EMERALD, "Pokémon SoulGold",
     PD_GUIDE_NONE},
    {"5d6a036260fdbde96f85b5c1b92d0256d3aebafb", &PD_CONFIG_SOULGOLD_V1_2B, PD_GAME_EMERALD, "Pokémon SoulGold",
     PD_GUIDE_NONE},
    {"2be7c99aa473308debe1d03ff2015bab7d9addb5", &PD_CONFIG_SOULGOLD_V1_2C, PD_GAME_EMERALD, "Pokémon SoulGold",
     PD_GUIDE_NONE},
    {"dacb6e40e7f572ddefd2dfa10e99c0da6835b062", &PD_CONFIG_SOULGOLD_V1_2C, PD_GAME_EMERALD, "Pokémon SoulGold (unverified)",
     PD_GUIDE_NONE},
    {"79ee6df0869c1773c8c6a5f764afc1f8d833d8bb", &PD_CONFIG_HEART_AND_SOUL, PD_GAME_EMERALD, "Pokémon Heart and Soul",
     PD_GUIDE_NONE},
    {"d0658315da1e8827f66f15c3d3a000fe747e163e", &PD_CONFIG_QUETZAL_V9_0, PD_GAME_EMERALD, "Pokémon Quetzal",
     PD_GUIDE_NONE},
    {"fe346b5b0eb022e3a103f81e7dcefba6f1dc542f", &PD_CONFIG_QUETZAL_V9_0_ES, PD_GAME_EMERALD, "Pokémon Quetzal",
     PD_GUIDE_NONE},
    {"c108cec750a35775084c0806cc704d8abd1994e7", &PD_CONFIG_QUETZAL_V9_1, PD_GAME_EMERALD, "Pokémon Quetzal",
     PD_GUIDE_NONE},
    {"ff008b70b6f2b330da9ffa8a405fc951a9e54147", &PD_CONFIG_QUETZAL_V9_1_ES, PD_GAME_EMERALD, "Pokémon Quetzal",
     PD_GUIDE_NONE},
    {"6806c7146aff457a4be85ef2030e9ddcdb17b665", &PD_CONFIG_QUETZAL_V0_8_4, PD_GAME_EMERALD, "Pokémon Quetzal",
     PD_GUIDE_NONE},
    {"b6542c7dbb40cf6ea96714a7a0e30cb468102861", &PD_CONFIG_QUETZAL_V0_8_4_ES, PD_GAME_EMERALD, "Pokémon Quetzal",
     PD_GUIDE_NONE},
    {"f1035f615c13c46394a18fa99181c24c3cd15e12", &PD_CONFIG_MOEMON_QUETZAL_V0_8_4, PD_GAME_EMERALD, "Moemon Quetzal",
     PD_GUIDE_NONE},
    {"ae5d88bb823a8f9d0d07e8784da2376ff7c1ae43", &PD_CONFIG_MOEMON_QUETZAL_V0_8_4_ES, PD_GAME_EMERALD, "Moemon Quetzal",
     PD_GUIDE_NONE},
    {NULL, NULL, PD_GAME_NONE, NULL, PD_GUIDE_NONE},
};

static bool retail_dump(const struct pd_game* g) {
    for (size_t i = 0; i < sizeof RETAIL_DUMPS / sizeof *RETAIL_DUMPS; i++) {
        const struct retail_dump* d = &RETAIL_DUMPS[i];
        if (strcmp(d->code, g->code) || d->rev != g->revision || g->romSize < d->end || g->romSize > 0x1000000) continue;
        static uint8_t chunk[1 << 16];
        struct pd_sha1 s;
        char hex[41];
        pd_sha1_init(&s);
        for (uint32_t off = 0; off < d->end; off += sizeof chunk) {
            uint32_t len = d->end - off < sizeof chunk ? d->end - off : sizeof chunk;
            memcpy(chunk, g->rom + off, len);
            for (int r = 0; r < 6 && d->ranges[r][1]; r++) {
                uint32_t a = d->ranges[r][0] > off ? d->ranges[r][0] : off;
                uint32_t z = d->ranges[r][1] < off + len ? d->ranges[r][1] : off + len;
                if (a < z) memset(chunk + (a - off), 0, z - a);
            }
            pd_sha1_update(&s, chunk, len);
        }
        pd_sha1_hex(&s, hex);
        if (!strcmp(hex, d->sha1)) return true;
    }
    return false;
}

void pd_game_identify(struct pd_game* g) {
    if (!g->rom || !g->kind) return;
    struct pd_sha1 s;
    pd_sha1_init(&s);
    pd_sha1_update(&s, g->rom, g->romSize);
    pd_sha1_hex(&s, g->sha1);

    for (const struct known_rom* k = KNOWN_ROMS; k->sha1; k++) {
        if (strcmp(k->sha1, g->sha1)) continue;
        g->cfg = k->cfg;
        g->kind = k->kind;
        g->title = k->title;
        g->guide = k->guide;
        g->hackMaps = k->cfg != &PD_CONFIG_EMERALD_LEGACY; // Legacy keeps Emerald's names (+ its own three)
        g->unsupported = NULL;
        return;
    }
    if (!g->cfg) return; // detect's verdict (a revision past 1, a big hack) stands
    // LeafGreen, Ruby and Sapphire are read by code and revision alone, like the app (OTHER_RETAIL_CODES):
    // only FireRed and Emerald, which most hacks build on, need a retail hash.
    if (strcmp(g->code, "BPRE") && strcmp(g->code, "BPEE")) return;
    for (size_t i = 0; i < sizeof RETAIL_SHA1S / sizeof *RETAIL_SHA1S; i++)
        if (!strcmp(RETAIL_SHA1S[i], g->sha1)) return;
    if (retail_dump(g)) return;
    g->cfg = NULL;
    g->guide = PD_GUIDE_NONE;
    g->title = g->kind == PD_GAME_FIRERED ? "FireRed ROM hack" : "Emerald ROM hack";
    g->unsupported = "Not a version the companion knows";
}
