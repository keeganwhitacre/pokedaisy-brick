/* Portable upstream experience progress, reusable by future game adapters. */
#ifndef POCKET_EXPERIENCE_H
#define POCKET_EXPERIENCE_H
#include <stdbool.h>
#include <stdint.h>
struct pocket_exp {
  uint32_t start, next, toNext;
  int filled;
};
/* Growth uses upstream Gen3 table ids; game-specific rate mappings stay in
 * adapters. */
bool pocket_exp_progress(int growth, int level, uint32_t total, int width,
                         struct pocket_exp *out);
#endif
