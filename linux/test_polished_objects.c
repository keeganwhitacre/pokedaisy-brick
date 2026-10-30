#include "../app/src/main/cpp/pk_wide.h"
#include "../app/src/main/cpp/pk_wide_gb_profile.h"
#include "polished_wide_tables.h"
int pc_lz(const unsigned char *, size_t, size_t, unsigned char *, size_t);
static int pocket_wide_configure(uint32_t crc) {
 return crc==PC_WIDE_CRC ? pk_wide_gb_configure_polished(pcWideConfig,pc_lz):pk_wide_gb_configure(crc);
}
/* GPL-3.0. Renderer model checks plus actual ROM/VRAM/PPU comparisons.
 * Model mutations touch caller-owned snapshots only, never running game RAM. */
#include "../3ds/desktop/pd_png.h"
#include "../app/src/main/cpp/pk_wide_gb.c"
#include <assert.h>
#include <fcntl.h>
#include <mgba-util/vfs.h>
#include <mgba/core/serialize.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t video[256 * 256], frame[160 * 144], out[256 * 192];
static struct PkWideView view = {-48, -24, 256, 192};
static bool screenshot(const char *path, const uint32_t *raw) {
  // mGBA is 0x00BBGGRR; pd_png_write expects 0x00RRGGBB, just like the
  // frontend's RGB888 texture. Keep raw buffers intact for PPU assertions.
  uint32_t rgb[256 * 192];
  for (int i = 0; i < 256 * 192; i++) {
    uint32_t n = raw[i];
    rgb[i] = ((n & 255) << 16) | (n & 0xff00) | ((n >> 16) & 255);
  }
  return pd_png_write(path, 256, 192, rgb);
}
static void tick(struct mCore *core, unsigned keys, int count) {
  core->setKeys(core, keys);
  for (int i = 0; i < count; i++) {
    core->runFrame(core);
    for (int y = 0; y < 144; y++)
      memcpy(frame + y * 160, video + y * 256, 640);
    pk_wide_gb_view(core, frame, out, view, NULL);
    for (int y = 0; y < 144; y++)
      assert(!memcmp(out + (y + 24) * 256 + 48, frame + y * 160, 640));
  }
}
int main(int argc, char **argv) {
  assert(argc == 3);
  struct VFile *rom = VFileOpen(argv[1], O_RDONLY);
  struct mCore *core = mCoreFindVF(rom);
  assert(core && core->init(core));
  mCoreInitConfig(core, NULL);
  core->setVideoBuffer(core, video, 256);
  assert(core->loadROM(core, rom));
  core->reset(core);
  struct VFile *state = VFileOpen(argv[2], O_RDONLY);
  assert(state && mCoreLoadStateNamed(core, state, SAVESTATE_RTC));
  state->close(state);
  assert(pocket_wide_configure(PC_WIDE_CRC));
  tick(core, 0, 60);
  if (getenv("POCKET_TEST_OUTDOOR")) {
    // Start from the normally earned aide-gift state at lab (4,8). Leave
    // through its real warp; no map/position/flag injection.
    struct GbSnap current;
    bool outside = false;
    for (int i = 0; i < 40 && !outside; i++) {
      tick(core, 128, 1);
      tick(core, 0, 17);
      gbTake(&current);
      outside = current.w == 10 && current.h == 9;
    }
    assert(outside);
    tick(core, 0, 120);
    assert(pk_wide_gb_last_score() >= GB_MIN_SCORE);
    assert(screenshot("runtime-phase18-new-bark.png", out));
    // Walk west along y=8, avoiding the teacher at (6,8), then cross into
    // Route29. The connection and NPCs are observed through real movement.
    for (int i = 0; i < 30 && gbRam(gbCfg[GB_YCOORD]) < 7; i++) {
      tick(core, 128, 1);
      tick(core, 0, 17);
    }
    assert(gbRam(gbCfg[GB_YCOORD]) >= 7);
    for (int i = 0; i < 5 && gbRam(gbCfg[GB_YCOORD] + 1) > 3; i++) {
      tick(core, 32, 1);
      tick(core, 0, 17);
    }
    for (int i = 0; i < 5 && gbRam(gbCfg[GB_YCOORD]) < 8; i++) {
      tick(core, 128, 1);
      tick(core, 0, 17);
    }
    bool route = false;
    for (int i = 0; i < 60 && !route; i++) {
      tick(core, 32, 1);
      tick(core, 0, 17);
      gbTake(&current);
      if (i % 10 == 0)
        fprintf(stderr, "west walk x=%d y=%d map=%dx%d\n", current.px,
                current.py, current.w, current.h);
      route = current.w == 30 && current.h == 9;
    }
    if (!route)
      screenshot("runtime-phase18-west-blocked.png", out);
    assert(route);
    tick(core, 0, 120);
    assert(pk_wide_gb_last_score() >= GB_MIN_SCORE);
    assert(screenshot("runtime-phase18-route29-connection.png", out));
    // Same live frame with only the previous loaded-object path, to expose
    // exactly what the new template layer contributes outside the PPU.
    struct GbSnap baseline = gbGood;
    memset(baseline.mapObjs, 0, sizeof baseline.mapObjs);
    for (int d = 0; d < 4; d++)
      baseline.conn[d].nObjs = 0;
    gbObjsFromOam();
    int ox = gbGoodTx * 8 + (gbSrcX & 7), oy = gbGoodTy * 8 + (gbSrcY & 7);
    gbObjsOffScreenCrystal(&baseline, ox, oy);
    uint32_t before[256 * 192];
    gbDraw(&baseline, ox, oy, -48, -24, 256, 192, before, 256, frame[0] & 0xff000000u,
           false);
    for (int y = 0; y < 144; y++)
      memcpy(before + (y + 24) * 256 + 48, frame + y * 160, 640);
    unsigned changed = 0;
    for (int i = 0; i < 256 * 192; i++)
      changed += before[i] != out[i];
    fprintf(stderr, "template-layer visible pixel changes: %u\n", changed);
    assert(changed);
    assert(screenshot("runtime-phase18-route29-templates-off.png", before));
    puts("PASS actual lab exit, NewBarkTown and west connection into Route29; "
         "original PPU preserved throughout");
  }
  assert(pk_wide_gb_last_score() >= GB_MIN_SCORE);
  assert(screenshot("runtime-phase18-wide-npcs.png", out));
  struct GbSnap snap;
  gbTake(&snap);
  assert(snap.ok);
  int unspawned = 0, added = 0;
  for (int i = 1; i < 21; i++) {
    const uint8_t *m = snap.mapObjs + i * 16;
    if (m[0] != 0xff || !m[1] || snap.masks[i])
      continue;
    unspawned++;
    gbNObjs = 0;
    pcWideMapObject(&snap, m, 200, 200, 0);
    if (gbNObjs)
      added++;
  }
  printf("Actual map: %d unspawned unmasked objects, %d supported ordinary "
         "sprites decoded\n",
         unspawned, added);
  // Actual player graphics must match the tiles the game decompressed into
  // VRAM. This checks bank, pointer, LZ codec and standing-frame tile order.
  const uint8_t *player = snap.objs;
  uint8_t m[14] = {0xff, player[0], 0, 0, 6, 0, 0, 0xff};
  gbNObjs = 0;
  pcWideMapObject(&snap, m, 200, 200, 0);
  assert(gbNObjs == 4 && gbObjectRomTiles[0]);
  unsigned tile = (player[2] & 0x7f) * 16 + ((player[2] & 0x80) ? 0 : 0x2000);
  assert(!memcmp(pcWideSprites[player[0]].tiles, gbB->video.vram + tile, 192));
  // constants/map_constants.asm: group 24, map 3 is Route29. Its real
  // script header has callbacks/coordinates, so Crystal's old sizes fail.
  const uint8_t *map = gbCrystalMapHeader(24, 3);
  assert(map);
  const uint8_t *attributes =
      gbRom(gbLinear(gbCfg[PC_MAP_ATTR_BANK], map[2] | map[3] << 8), 10);
  assert(attributes);
  struct GbConn connected = {.on = true};
  pcWideConnectionEvents(&connected, attributes);
  assert(connected.nObjs >= 8);
  const uint8_t *lyra = gbRom(connected.objs, 13);
  assert(lyra && lyra[0] == 9 && lyra[1] == 16 && lyra[2] == 54 &&
         lyra[3] == 3);
  puts("PASS actual Route29 connected-map event parser "
       "(scene/callback/warp/coordinate/background lists)");
  // Model cases stay independent of the actual emulator's memory.
  struct GbSnap model = snap;
  memset(model.mapObjs, 0, sizeof model.mapObjs);
  memset(model.masks, 0, sizeof model.masks);
  memset(model.conn, 0, sizeof model.conn);
  memcpy(model.mapObjs + 16, m, 14);
  model.mapObjs[16 + 12] = model.mapObjs[16 + 13] = 0xff;
  gbNObjs = 0;
  pcWideObjects(&model, 0, 0, 0);
  assert(gbNObjs == 4);
  model.masks[1] = 1;
  uint8_t savedObjects[sizeof model.objs];
  memcpy(savedObjects, model.objs, sizeof savedObjects);
  memset(model.objs, 0, sizeof model.objs);
  model.objs[40] = m[1]; // carryover actor with same initial coordinates
  assert(pcWideLoadedAt(&model, m[1], 0, 0));
  model.masks[1] = 0;
  gbNObjs = 0;
  pcWideObjects(&model, 0, 0, 0);
  assert(!gbNObjs);
  memcpy(model.objs, savedObjects, sizeof savedObjects);
  model.masks[1] = 1;
  gbNObjs = 0;
  pcWideObjects(&model, 0, 0, 0);
  assert(!gbNObjs);
  model.masks[1] = 0;
  model.mapObjs[16] = 2; // already associated: no duplicate
  gbNObjs = 0;
  pcWideObjects(&model, 0, 0, 0);
  assert(!gbNObjs);
  model.mapObjs[16] = 0xff;
  model.mapObjs[16 + 7] = 0; // no allowed time
  gbNObjs = 0;
  pcWideObjects(&model, 0, 0, 0);
  assert(!gbNObjs);
  assert(!pcWideObjectNow(0xff, 0xff00) && !pcWideObjectNow(0xff, 2303));
  memset(model.loadedPals, 0xff, sizeof model.loadedPals);
  gbNObjs = 0;
  pcWideMapObject(&model, m, 200, 200, 0);
  assert(!gbNObjs);
  m[1] = 0xef;
  gbNObjs = 0;
  pcWideMapObject(&snap, m, 200, 200, 0);
  assert(!gbNObjs);
  m[1] = player[0];
  m[4] = 0x2a;
  gbNObjs = 0;
  pcWideMapObject(&snap, m, 200, 200, 0);
  assert(!gbNObjs);
  pcWideSpriteCacheReset();
  assert(!pcWideSprites[player[0]].state);
  puts("PASS actual ROM graphics equal original VRAM; every original PPU pixel "
       "preserved; masks, time, association, palette, special-sprite and cache "
       "guards");
  core->unloadROM(core);
  mCoreConfigDeinit(&core->config);
  core->deinit(core);
}
