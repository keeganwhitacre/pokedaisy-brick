/* GPL-3.0. FavoriteItems.kt's per-ROM item-id set. No game-memory access. */
#ifndef POCKET_FAVORITES_H
#define POCKET_FAVORITES_H
#include <stdbool.h>
#include <stdint.h>
struct pocket_favorites {
  uint8_t bits[8192];
};
static inline bool pocket_favorite_has(const struct pocket_favorites *f,
                                       uint16_t id) {
  return id && (f->bits[id / 8] & (1u << (id % 8)));
}
static inline void pocket_favorite_toggle(struct pocket_favorites *f,
                                          uint16_t id) {
  if (id)
    f->bits[id / 8] ^= 1u << (id % 8);
}
/* Linux storage adapter; missing/invalid files produce an empty set. */
bool pocket_favorites_load(const char *root, const char *sha,
                           struct pocket_favorites *);
bool pocket_favorites_save(const char *root, const char *sha,
                           const struct pocket_favorites *);
#endif
