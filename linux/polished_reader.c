/* GPL-3.0. Compatibility entry point; one implementation owns Polished decoding. */
#include "polished_reader.h"
#include "polished.h"
#include "pd_tables.h"
bool pc_snapshot_read(struct pd_snapshot *s, pd_read_fn read, void *ctx) {
    bool ok=pc_read(s, read, ctx);
    for (int i=0; i<s->partyCount; i++)
        if (s->party[i].species==255) s->party[i].species=PD_SPECIES_EGG;
    return ok;
}
