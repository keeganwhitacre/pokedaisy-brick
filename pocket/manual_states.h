/* GPL-3.0. Linux adapter of SaveStates.kt, using mGBA's extended state format.
 */
#ifndef POCKET_MANUAL_STATES_H
#define POCKET_MANUAL_STATES_H
#include <stdbool.h>
#include <stdint.h>
struct mCore;
enum pocket_slot_status { SLOT_EMPTY, SLOT_READY, SLOT_LEGACY, SLOT_INVALID };
struct pocket_slot {
  enum pocket_slot_status status;
  int width, height;
  int64_t savedAt;
  uint32_t preview[240 * 160]; /* RGB888, never raw mGBA channel order. */
};
bool pocket_slot_path(const char *quick, int index, char path[1200]);
void pocket_slot_read(struct mCore *, const char *path, const char *rom,
                      struct pocket_slot *);
bool pocket_slot_save(struct mCore *, const char *path, const char *rom);
bool pocket_slot_load(struct mCore *, const char *path, const char *rom);
#endif
