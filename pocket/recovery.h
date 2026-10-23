/* GPL-3.0. Emulator recovery, independent of game decoding and SDL. */
#ifndef POCKET_RECOVERY_H
#define POCKET_RECOVERY_H
#include <stdbool.h>
struct mCore;
enum pocket_recovery_result { RECOVERY_MISSING, RECOVERY_LOADED,
                              RECOVERY_INVALID, RECOVERY_STALE };
bool pocket_recovery_save(struct mCore *, const char *root, const char *sha,
                          const char *battery);
enum pocket_recovery_result pocket_recovery_load(struct mCore *, const char *root,
                                                  const char *sha, const char *battery);
#endif
