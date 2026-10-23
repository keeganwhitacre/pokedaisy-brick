/* GPL-3.0. Prefs.favoriteItemsFor/setFavoriteItemsFor Linux adaptation.
 * Sorted decimal ids as upstream; SHA-1 ROM identity matches Pocket settings.
 * Preferences are separate from saves, states and recovery sessions. */
#define _POSIX_C_SOURCE 200809L
#include "favorites.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static bool paths(const char *root, const char *sha, char dir[1200],
                  char path[1300]) {
  if (!root || !sha || strlen(sha) != 40)
    return false;
  for (int i = 0; i < 40; i++)
    if (!((sha[i] >= '0' && sha[i] <= '9') || (sha[i] >= 'a' && sha[i] <= 'f')))
      return false;
  return snprintf(dir, 1200, "%s/favorites", root) < 1200 &&
         snprintf(path, 1300, "%s/%s.txt", dir, sha) < 1300;
}
bool pocket_favorites_load(const char *root, const char *sha,
                           struct pocket_favorites *f) {
  memset(f, 0, sizeof *f);
  char dir[1200], path[1300], header[32];
  if (!paths(root, sha, dir, path))
    return false;
  FILE *in = fopen(path, "r");
  if (!in)
    return false;
  struct pocket_favorites next = {0};
  bool ok =
      fgets(header, sizeof header, in) && !strcmp(header, "PD-FAVORITES-1\n");
  unsigned id = 0, digits = 0, bytes = 0;
  int c = EOF;
  while (ok && (c = fgetc(in)) != EOF) {
    if (++bytes > 393216) {
      ok = false;
      break;
    }
    if (c >= '0' && c <= '9') {
      id = id * 10 + c - '0';
      if (++digits > 5 || id > 65535)
        ok = false;
    } else if (c == ',' || c == '\n') {
      if (digits && id)
        next.bits[id / 8] |= 1u << (id % 8);
      else if (c != '\n' || bytes != 1)
        ok = false;
      id = digits = 0;
      if (c == '\n') {
        ok = ok && fgetc(in) == EOF;
        break;
      }
    } else
      ok = false;
  }
  ok = ok && c == '\n' && !ferror(in);
  if (fclose(in))
    ok = false;
  if (ok)
    *f = next;
  return ok;
}
bool pocket_favorites_save(const char *root, const char *sha,
                           const struct pocket_favorites *f) {
  char dir[1200], path[1300], temp[1320];
  if (!paths(root, sha, dir, path))
    return false;
  mkdir(dir, 0755);
  snprintf(temp, sizeof temp, "%s.XXXXXX", path);
  int fd = mkstemp(temp);
  if (fd < 0)
    return false;
  FILE *out = fdopen(fd, "w");
  if (!out) {
    close(fd);
    unlink(temp);
    return false;
  }
  bool ok = fputs("PD-FAVORITES-1\n", out) >= 0, first = true;
  for (unsigned id = 1; ok && id <= 65535; id++)
    if (pocket_favorite_has(f, (uint16_t)id)) {
      ok = fprintf(out, "%s%u", first ? "" : ",", id) > 0;
      first = false;
    }
  if (fputc('\n', out) == EOF || fflush(out) || fsync(fd))
    ok = false;
  if (fclose(out))
    ok = false;
  if (ok)
    ok = rename(temp, path) == 0;
  if (ok) {
    int d = open(dir, O_RDONLY | O_DIRECTORY);
    ok = d >= 0 && fsync(d) == 0;
    if (d >= 0 && close(d))
      ok = false;
  }
  if (!ok)
    unlink(temp);
  return ok;
}
