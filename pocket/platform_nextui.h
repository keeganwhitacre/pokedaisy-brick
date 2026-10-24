/* GPL-3.0. NextUI foreground power adapter; no SDL or emulator dependency. */
#ifndef POCKET_PLATFORM_NEXTUI_H
#define POCKET_PLATFORM_NEXTUI_H
#include <stdbool.h>
#include <stdint.h>
struct pocket_platform {
  bool active, powerDown, sleepRequested, shutdownRequested;
  uint32_t pressedAt, ignoreUntil;
  int fds[32], count;
  bool dropped[32];
  char suspendHelper[1200];
};
void pocket_platform_init(struct pocket_platform *);
void pocket_platform_close(struct pocket_platform *);
void pocket_platform_key(struct pocket_platform *, bool down, uint32_t now);
void pocket_platform_poll(struct pocket_platform *, uint32_t now);
void pocket_platform_woke(struct pocket_platform *, uint32_t now);
bool pocket_platform_suspend(struct pocket_platform *);
bool pocket_platform_poweroff(struct pocket_platform *);
/* Read-only start/exit diagnostics; no per-frame battery/sysfs polling. */
void pocket_platform_log_energy(const struct pocket_platform *, const char *);
#endif
