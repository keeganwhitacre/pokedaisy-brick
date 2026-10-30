#ifndef PD_POLISHED_READER_H
#define PD_POLISHED_READER_H
#include "pd_snapshot.h"
/* Caller must first identify a supported Polished Crystal ROM. No writes. */
bool pc_snapshot_read(struct pd_snapshot *, pd_read_fn, void *);
#endif
