// Read-only Gen 3 summary data; order matches Android MonStats.kt.
#ifndef PD_STATS_H
#define PD_STATS_H

#include <stdbool.h>
#include <stdint.h>

struct pd_mon_stats {
    bool valid;
    uint8_t ivs[6], evs[6]; // HP, Attack, Defense, Speed, Sp.Atk, Sp.Def
    int values[6], nature;
};

int pd_nature_raises(int nature); // struct index, -1 for neutral/unknown
int pd_nature_lowers(int nature);
int pd_hidden_power_type(const struct pd_mon_stats* stats);
int pd_hidden_power_power(const struct pd_mon_stats* stats); // retail Gen 3

#endif
