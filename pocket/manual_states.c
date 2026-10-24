/* GPL-3.0. One atomic mGBA state contains screenshot, timestamp and ROM/hash
 * metadata. No sidecar pairing race; no savedata serialization/restoration. */
#define _POSIX_C_SOURCE 200809L
#include "manual_states.h"
#include "rom_identity.h"
#include <errno.h>
#include <fcntl.h>
#include <mgba-util/memory.h>
#include <mgba-util/vfs.h>
#include <mgba/core/core.h>
#include <mgba/core/serialize.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define MAX_FILE (16 * 1024 * 1024)
bool pocket_slot_path(const char *quick, int index, char path[1200]) {
  if (index < 0 || index > 9)
    return false;
  return (index ? snprintf(path, 1200, "%s.slot%d", quick, index)
                : snprintf(path, 1200, "%s", quick)) < 1200;
}
static bool rom_key(const char *rom) {
  if (!rom || strlen(rom) != 40)
    return false;
  for (int i = 0; i < 40; i++)
    if (!((rom[i] >= '0' && rom[i] <= '9') || (rom[i] >= 'a' && rom[i] <= 'f')))
      return false;
  return true;
}
static bool metadata(struct mCore *c, struct mStateExtdata *ext, void *state,
                     const char *rom, bool create) {
  unsigned w, h;
  c->desiredVideoDimensions(c, &w, &h);
  struct mStateExtdataItem image, creator, savedata;
  if (!rom_key(rom) || w > 240 || h > 160 || !w || !h ||
      !mStateExtdataGet(ext, EXTDATA_SCREENSHOT, &image) || !image.data ||
      image.size != (int)(w * h * 4))
    return false;
  if (mStateExtdataGet(ext, EXTDATA_SAVEDATA, &savedata) && savedata.data)
    return false;
  char stateHash[41], imageHash[41], text[160];
  pocket_rom_sha1(state, c->stateSize(c), stateHash);
  pocket_rom_sha1(image.data, (size_t)image.size, imageHash);
  snprintf(text, sizeof text, "PDPOCKET-SLOT1\n%s\n%s\n%s", rom, stateHash,
           imageHash);
  if (create) {
    creator = (struct mStateExtdataItem){
        .size = (int)strlen(text) + 1, .data = strdup(text), .clean = free};
    if (!creator.data)
      return false;
    mStateExtdataPut(ext, EXTDATA_META_CREATOR, &creator);
    return true;
  }
  return mStateExtdataGet(ext, EXTDATA_META_CREATOR, &creator) &&
         creator.data && creator.size == (int)strlen(text) + 1 &&
         !memcmp(creator.data, text, (size_t)creator.size);
}
static void *extract(struct mCore *c, const char *path,
                     struct mStateExtdata *ext, struct stat *st) {
  if (stat(path, st) || !S_ISREG(st->st_mode) || st->st_size <= 0 ||
      st->st_size > MAX_FILE)
    return NULL;
  struct VFile *f = VFileOpen(path, O_RDONLY);
  bool valid = f && st->st_size >= (off_t)c->stateSize(c);
  if (valid && st->st_size != (off_t)c->stateSize(c)) {
    valid = false;
    f->seek(f, c->stateSize(c), SEEK_SET);
    for (int i = 0; i < EXTDATA_MAX; i++) {
      unsigned char h[16];
      if (f->read(f, h, 16) != 16)
        break;
      uint32_t tag = 0, size = 0;
      uint64_t at = 0;
      for (int j = 3; j >= 0; j--) {
        tag = (tag << 8) | h[j];
        size = (size << 8) | h[j + 4];
      }
      for (int j = 15; j >= 8; j--)
        at = (at << 8) | h[j];
      if (!tag) {
        valid = true;
        break;
      }
      if (tag >= EXTDATA_MAX || !size || size > MAX_FILE ||
          at > (uint64_t)st->st_size || size > (uint64_t)st->st_size - at)
        break;
    }
  }
  void *state = valid ? mCoreExtractState(c, f, ext) : NULL;
  if (f)
    f->close(f);
  return state;
}
void pocket_slot_read(struct mCore *c, const char *path, const char *rom,
                      struct pocket_slot *slot) {
  memset(slot, 0, sizeof *slot);
  struct stat st;
  if (stat(path, &st)) {
    slot->status = errno == ENOENT ? SLOT_EMPTY : SLOT_INVALID;
    return;
  }
  slot->status = SLOT_INVALID;
  slot->savedAt = st.st_mtime;
  struct mStateExtdata ext;
  mStateExtdataInit(&ext);
  void *state = extract(c, path, &ext, &st);
  struct mStateExtdataItem creator, image, savedata;
  if (state) {
    if (metadata(c, &ext, state, rom, false)) {
      unsigned w, h;
      c->desiredVideoDimensions(c, &w, &h);
      slot->width = w;
      slot->height = h;
      slot->status = SLOT_READY;
      mStateExtdataGet(&ext, EXTDATA_SCREENSHOT, &image);
      const uint32_t *raw = image.data;
      for (unsigned i = 0; i < w * h; i++) {
        uint32_t n = raw[i];
        slot->preview[i] = ((n & 255) << 16) | (n & 0xff00) | ((n >> 16) & 255);
      }
    } else if ((!mStateExtdataGet(&ext, EXTDATA_SAVEDATA, &savedata) ||
                !savedata.data) &&
               (!mStateExtdataGet(&ext, EXTDATA_META_CREATOR, &creator) ||
                !creator.data) &&
               (!mStateExtdataGet(&ext, EXTDATA_SCREENSHOT, &image) ||
                !image.data))
      slot->status =
          SLOT_LEGACY; // old raw state: loading still validated by core
  }
  if (state)
    mappedMemoryFree(state, c->stateSize(c));
  mStateExtdataDeinit(&ext);
}
bool pocket_slot_save(struct mCore *c, const char *path, const char *rom) {
  if (!rom_key(rom))
    return false;
  struct VFile *mem = VFileMemChunk(NULL, 0);
  if (!mem)
    return false;
  bool ok = mCoreSaveStateNamed(c, mem, SAVESTATE_RTC | SAVESTATE_METADATA);
  struct mStateExtdata ext;
  mStateExtdataInit(&ext);
  mem->seek(mem, 0, SEEK_SET);
  void *state = ok ? mCoreExtractState(c, mem, &ext) : NULL;
  mem->close(mem);
  unsigned w, h;
  c->desiredVideoDimensions(c, &w, &h);
  const void *pixels = NULL;
  size_t stride = 0;
  c->getPixels(c, &pixels, &stride);
  uint32_t *image = w && h && w <= 240 && h <= 160 ? malloc(w * h * 4) : NULL;
  ok = state && image && pixels && stride >= w;
  if (ok) {
    for (unsigned y = 0; y < h; y++)
      memcpy(image + y * w, (const uint32_t *)pixels + y * stride, w * 4);
    struct mStateExtdataItem item = {
        .size = (int)(w * h * 4), .data = image, .clean = free};
    mStateExtdataPut(&ext, EXTDATA_SCREENSHOT, &item);
    image = NULL;
    ok = metadata(c, &ext, state, rom, true);
  }
  free(image);
  char temp[1240];
  int fd = -1;
  if (snprintf(temp, sizeof temp, "%s.tmp-XXXXXX", path) >= (int)sizeof temp)
    ok = false;
  if (ok) {
    fd = mkstemp(temp);
    ok = fd >= 0;
  }
  struct VFile *out = ok ? VFileFromFD(fd) : NULL;
  if (out) {
    ok = out->write(out, state, c->stateSize(c)) == (ssize_t)c->stateSize(c) &&
         mStateExtdataSerialize(&ext, out);
    if (!out->sync(out, NULL, 0))
      ok = false;
    if (!out->close(out))
      ok = false;
  } else {
    if (fd >= 0)
      close(fd);
    ok = false;
  }
  if (state)
    mappedMemoryFree(state, c->stateSize(c));
  mStateExtdataDeinit(&ext);
  if (ok)
    ok = rename(temp, path) == 0;
  if (!ok) {
    if (fd >= 0)
      unlink(temp);
    return false;
  }
  char dir[1200];
  snprintf(dir, sizeof dir, "%s", path);
  char *slash = strrchr(dir, '/');
  if (!slash)
    return false;
  *slash = 0;
  fd = open(dir, O_RDONLY | O_DIRECTORY);
  ok = fd >= 0 && fsync(fd) == 0;
  if (fd >= 0 && close(fd))
    ok = false;
  return ok;
}
bool pocket_slot_load(struct mCore *c, const char *path, const char *rom) {
  struct pocket_slot *slot = malloc(sizeof *slot);
  if (!slot)
    return false;
  pocket_slot_read(c, path, rom, slot);
  bool ok = slot->status == SLOT_READY || slot->status == SLOT_LEGACY;
  free(slot);
  struct VFile *f = ok ? VFileOpen(path, O_RDONLY) : NULL;
  void *rollback = malloc(c->stateSize(c));
  ok = f && rollback && c->saveState(c, rollback);
  if (ok && !mCoreLoadStateNamed(c, f, SAVESTATE_RTC | SAVESTATE_SCREENSHOT)) {
    c->loadState(c, rollback);
    ok = false;
  }
  free(rollback);
  if (f)
    f->close(f);
  return ok;
}
