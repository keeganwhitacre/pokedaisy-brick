// Game Boy games (Gen 1 and Gen 2): where each keeps its party, bag and battle in WRAM, and the reader that turns it
// into a struct pd_snapshot (the app's Gen1Reader.kt / Gen2Reader.kt). Addresses: a bus address (C000-CFFF), or
// WRAM bank b's D000-DFFF as (b << 16) | address (the host reads that bank whatever SVBK says).
#ifndef PD_GB_H
#define PD_GB_H

#include <stdbool.h>
#include <stdint.h>

#include "pd_snapshot.h"

#define PD_WRAMX(bank, addr) (((uint32_t) (bank) << 16) | (addr))

struct pd_gb_config {
    int gen; // 1 or 2
    uint32_t partyCount, partyMons;
    // Gen 1: wNumBagItems (then {item, quantity} pairs). Gen 2: wNumItems, wNumKeyItems, wNumBalls, wTMsHMs.
    uint32_t numItems, numKeyItems, numBalls, tmsHms;
    uint32_t money;
    uint32_t battleMode; // Gen 1's wIsInBattle: 0 none, 1 wild, 2 trainer
    uint32_t battleMon, enemyMon, otPartyCount, otPartyMons, curOTMon;
    const uint16_t* internalToDex; // Gen 1: the internal species index -> Dex number
    int internalCount;
    const uint16_t* genderRatios; // Gen 2, by species id
    int genderCount;
    const uint16_t* tmItems; // Gen 2: the item id of each TM / HM
    int tmCount;
    int speciesCount;        // Gen 2: the highest species id
    int itemsMax, ballsMax, keyItemsMax, bagSlots; // pocket sizes (Gen 1: bagSlots)
    bool tmBits;             // wTMsHMs is a bit per TM, not a quantity
    bool polished;           // Polished Crystal's own structs, pockets and charmap
    uint32_t numMedicine, numBerries;
    int medicineMax, berriesMax;
    // POKéDEX flags: wPokedexOwned / wPokedexSeen, a bit per Dex number from 1 (Gen 2: by species id); dexCount 0 = no DEX.
    uint32_t dexCaught, dexSeen;
    int dexCount;
};

extern const uint32_t pd_gb_polished_variants[];
extern const int pd_gb_polished_variants_count;

extern const struct pd_gb_config pd_gb_yellow, pd_gb_red, pd_gb_pure_red, pd_gb_crystal, pd_gb_gold, pd_gb_prism,
    pd_gb_polished, pd_gb_crystal_clear;

struct pd_config;
void pd_gb_snapshot_read(struct pd_snapshot* s, const struct pd_config* cfg, pd_read_fn read, void* ctx);

#endif
