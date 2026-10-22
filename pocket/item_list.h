/* GPL-3.0. ItemsScreen's read-only ordering, independent of game memory/UI.
 * Keep distinct stacks distinct; sorting never mutates an emulated bag. */
#ifndef POCKET_ITEM_LIST_H
#define POCKET_ITEM_LIST_H
#include <stdint.h>
#include <string.h>
struct pocket_item_ref {
  const char *name;
  uint16_t id, quantity, pocket, slot;
};
enum pocket_item_sort {
  POCKET_SORT_DEFAULT,
  POCKET_SORT_NAME,
  POCKET_SORT_TYPE,
  POCKET_SORT_COUNT
};
static inline int pocket_item_compare(const struct pocket_item_ref *a,
                                      const struct pocket_item_ref *b,
                                      int sort) {
  if (sort == POCKET_SORT_COUNT && a->quantity != b->quantity)
    return a->quantity > b->quantity ? -1 : 1;
  if (sort == POCKET_SORT_TYPE && a->pocket != b->pocket)
    return a->pocket < b->pocket ? -1 : 1;
  return sort == POCKET_SORT_DEFAULT ? 0 : strcmp(a->name, b->name);
}
/* Stable mergesort, caller-owned scratch: bounded O(n log n), no allocation. */
static inline void pocket_item_sort(struct pocket_item_ref *rows,
                                    struct pocket_item_ref *scratch, int count,
                                    int sort) {
  if (sort <= POCKET_SORT_DEFAULT || sort > POCKET_SORT_COUNT || count < 2)
    return;
  for (int step = 1; step < count; step *= 2) {
    for (int start = 0; start < count; start += 2 * step) {
      int mid = start + step < count ? start + step : count;
      int end = mid + step < count ? mid + step : count;
      int a = start, b = mid;
      for (int out = start; out < end; out++)
        scratch[out] =
            b == end || (a < mid &&
                         pocket_item_compare(rows + a, rows + b, sort) <= 0)
                ? rows[a++]
                : rows[b++];
    }
    memcpy(rows, scratch, (size_t)count * sizeof *rows);
  }
}
#endif
