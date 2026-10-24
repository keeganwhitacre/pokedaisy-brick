/* GPL-3.0. Atomic host envelope around mGBA states. Never restore savedata.
 * Fingerprint SRAM content, excluding the GB RTC trailer updated on exit. */
#include "recovery.h"
#include "rom_identity.h"
#include <errno.h>
#include <fcntl.h>
#include <mgba-util/vfs.h>
#include <mgba/core/core.h>
#include <mgba/core/serialize.h>
#include <mgba/internal/gb/gb.h>
#include <mgba/internal/gba/gba.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define HEADER 144
#define MAX_STATE (16 * 1024 * 1024)
static bool paths(const char *root, const char *sha, char *dir, char *path) {
  if (!sha || strlen(sha) != 40)
    return false;
  for (int i = 0; i < 40; i++)
    if (!((sha[i] >= '0' && sha[i] <= '9') || (sha[i] >= 'a' && sha[i] <= 'f')))
      return false;
  return snprintf(dir, 1200, "%s/recovery", root) < 1200 &&
         snprintf(path, 1200, "%s/%s.session", dir, sha) < 1200;
}
static size_t battery_memory(struct mCore *c, struct VFile **vf, void **ram) {
  if (c->platform(c) == mPLATFORM_GB) {
    struct GB *g = c->board;
    *vf = g->sramVf;
    *ram = g->memory.sram;
    return g->sramSize;
  }
  struct GBA *g = c->board;
  *vf = g->memory.savedata.vf;
  *ram = g->memory.savedata.data;
  return GBASavedataSize(&g->memory.savedata);
}
static bool battery_hash(const char *path, size_t n, char sha[41]) {
  if (n > 4 * 1024 * 1024)
    return false;
  unsigned char *p = malloc(n ? n : 1);
  if (!p)
    return false;
  FILE *f = fopen(path, "rb");
  bool ok = f && fread(p, 1, n, f) == n && !ferror(f);
  if (f && fclose(f))
    ok = false;
  if (ok)
    pocket_rom_sha1(p, n, sha);
  free(p);
  return ok;
}
static void put64(unsigned char *p, uint64_t n) {
  for (int i = 0; i < 8; i++) {
    p[i] = n & 255;
    n >>= 8;
  }
}
static uint64_t get64(const unsigned char *p) {
  uint64_t n = 0;
  for (int i = 7; i >= 0; i--)
    n = (n << 8) | p[i];
  return n;
}
bool pocket_recovery_save(struct mCore *c, const char *root, const char *sha,
                          const char *battery) {
  char dir[1200], path[1200], tmp[1240], hash[41];
  if (!paths(root, sha, dir, path))
    return false;
  struct VFile *sv = NULL;
  void *ram = NULL;
  size_t ramSize = battery_memory(c, &sv, &ram);
  if (ramSize && (!sv || !ram || !sv->sync(sv, ram, ramSize)))
    return false;
  int batteryFd = open(battery, O_RDONLY | O_CLOEXEC);
  if (batteryFd < 0)
    return false;
  bool ok = fsync(batteryFd) == 0;
  close(batteryFd);
  if (!ok)
    return false;
  unsigned char h[HEADER] = {0};
  memcpy(h, "PDREC01\n", 8);
  memcpy(h + 8, sha, 40);
  if (!battery_hash(battery, ramSize, hash))
    return false;
  memcpy(h + 48, hash, 40);
  put64(h + 128, ramSize);
  struct VFile *mem = VFileMemChunk(NULL, 0);
  if (!mem)
    return false;
  ok = mCoreSaveStateNamed(c, mem, SAVESTATE_RTC);
  ssize_t n = mem->size(mem);
  unsigned char *bytes = n > 0 && n <= MAX_STATE ? malloc((size_t)n) : NULL;
  ok = ok && bytes && mem->seek(mem, 0, SEEK_SET) == 0 &&
       mem->read(mem, bytes, (size_t)n) == n;
  mem->close(mem);
  if (!ok) {
    free(bytes);
    return false;
  }
  pocket_rom_sha1(bytes, (size_t)n, hash);
  memcpy(h + 88, hash, 40);
  put64(h + 136, (uint64_t)n);
  if (mkdir(dir, 0755) && errno != EEXIST) {
    free(bytes);
    return false;
  }
  snprintf(tmp, sizeof tmp, "%s.tmp-XXXXXX", path);
  int fd = mkstemp(tmp);
  FILE *f = fd >= 0 ? fdopen(fd, "wb") : NULL;
  if (!f) {
    if (fd >= 0) {
      close(fd);
      unlink(tmp);
    }
    free(bytes);
    return false;
  }
  ok = fwrite(h, 1, HEADER, f) == HEADER &&
       fwrite(bytes, 1, (size_t)n, f) == (size_t)n;
  free(bytes);
  if (fflush(f) || fsync(fd))
    ok = false;
  if (fclose(f))
    ok = false;
  if (ok)
    ok = rename(tmp, path) == 0;
  if (!ok)
    unlink(tmp);
  else {
    int d = open(dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (d >= 0) {
      ok = fsync(d) == 0;
      close(d);
    } else
      ok = false;
  }
  return ok;
}
enum pocket_recovery_result pocket_recovery_load(struct mCore *c,
                                                 const char *root,
                                                 const char *sha,
                                                 const char *battery) {
  char dir[1200], path[1200], hash[41];
  unsigned char h[HEADER];
  if (!paths(root, sha, dir, path))
    return RECOVERY_INVALID;
  FILE *f = fopen(path, "rb");
  if (!f)
    return errno == ENOENT ? RECOVERY_MISSING : RECOVERY_INVALID;
  bool ok = fread(h, 1, HEADER, f) == HEADER;
  if (!ok || memcmp(h, "PDREC01\n", 8) || memcmp(h + 8, sha, 40)) {
    fclose(f);
    return RECOVERY_INVALID;
  }
  uint64_t n = get64(h + 136), savedSize = get64(h + 128);
  struct VFile *sv = NULL;
  void *ram = NULL;
  size_t ramSize = battery_memory(c, &sv, &ram);
  if (savedSize != ramSize || !battery_hash(battery, ramSize, hash) ||
      memcmp(h + 48, hash, 40)) {
    fclose(f);
    return RECOVERY_STALE;
  }
  if (!n || n > MAX_STATE) {
    fclose(f);
    return RECOVERY_INVALID;
  }
  unsigned char *bytes = malloc((size_t)n);
  if (!bytes) {
    fclose(f);
    return RECOVERY_INVALID;
  }
  ok = fread(bytes, 1, (size_t)n, f) == n && fgetc(f) == EOF && !ferror(f);
  if (fclose(f))
    ok = false;
  if (ok) {
    pocket_rom_sha1(bytes, (size_t)n, hash);
    ok = !memcmp(h + 88, hash, 40);
  }
  if (ok) {
    struct VFile *mem = VFileFromConstMemory(bytes, (size_t)n);
    ok = mem && mCoreLoadStateNamed(c, mem, SAVESTATE_RTC);
    if (mem)
      mem->close(mem);
    if (!ok)
      c->reset(c);
  }
  free(bytes);
  return ok ? RECOVERY_LOADED : RECOVERY_INVALID;
}
