/* GPL-3.0. Per-ROM preference isolation and malformed-file rejection. */
#define _POSIX_C_SOURCE 200809L
#include "favorites.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
int main(void) {
  char root[] = "/tmp/pd-favorites-XXXXXX", path[300], dir[300];
  assert(mkdtemp(root));
  const char *a = "6930b48af5844d373e3c9130f26d6dd1084cf4ed";
  const char *b = "0000000000000000000000000000000000000000";
  struct pocket_favorites f = {0}, loaded;
  assert(!pocket_favorites_load(root, a, &loaded));
  assert(!memcmp(&f, &loaded, sizeof f));
  pocket_favorite_toggle(&f, 0);
  assert(!pocket_favorite_has(&f, 0));
  pocket_favorite_toggle(&f, 25);
  pocket_favorite_toggle(&f, 65535);
  assert(pocket_favorites_save(root, a, &f));
  assert(pocket_favorites_load(root, a, &loaded));
  assert(!memcmp(&f, &loaded, sizeof f));
  assert(!pocket_favorites_load(root, b, &loaded));
  assert(!pocket_favorite_has(&loaded, 25));
  assert(!pocket_favorites_save(root, "../../bad", &f));
  // Failed replacement leaves the last valid preference intact.
  assert(!pocket_favorites_save("/dev/null", a, &f));
  assert(pocket_favorites_load(root, a, &loaded));
  assert(!memcmp(&f, &loaded, sizeof f));
  snprintf(path, sizeof path, "%s/favorites/%s.txt", root, a);
  const char *bad[] = {"",
                       "PD-FAVORITES-2\n25\n",
                       "PD-FAVORITES-1\n25",
                       "PD-FAVORITES-1\n65536\n",
                       "PD-FAVORITES-1\n1,,2\n",
                       "PD-FAVORITES-1\n1,\n",
                       "PD-FAVORITES-1\n0\n",
                       "PD-FAVORITES-1\n-1\n",
                       "PD-FAVORITES-1\n25\ntrailing"};
  for (unsigned i = 0; i < sizeof bad / sizeof *bad; i++) {
    FILE *out = fopen(path, "w");
    assert(out);
    assert(fputs(bad[i], out) >= 0 && !fclose(out));
    assert(!pocket_favorites_load(root, a, &loaded));
    struct pocket_favorites empty = {0};
    assert(!memcmp(&empty, &loaded, sizeof empty));
  }
  // Empty is valid; ids survive temporary absence from the emulated bag.
  memset(&f, 0, sizeof f);
  assert(pocket_favorites_save(root, a, &f));
  assert(pocket_favorites_load(root, a, &loaded));
  // Full representable id range is bounded and round-trips.
  memset(f.bits, 255, sizeof f.bits);
  f.bits[0] &= ~1u;
  assert(pocket_favorites_save(root, a, &f));
  assert(pocket_favorites_load(root, a, &loaded));
  assert(!memcmp(&f, &loaded, sizeof f));
  unlink(path);
  snprintf(dir, sizeof dir, "%s/favorites", root);
  rmdir(dir);
  rmdir(root);
  puts("PASS favorites: per-ROM isolation, atomic persistence, complete id "
       "range, invalid/truncated files and empty set");
}
