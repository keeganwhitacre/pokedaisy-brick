/* GPL-3.0. Adapted from NextUI PWR_update/PWR_deepSleep, tg5040 platform
 * and launcher. keymon owns volume/brightness, not foreground power.
 * No EVIOCGRAB, daemon replacement, fake sleep or direct shutdown syscall. */
#include "platform_nextui.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
void pocket_platform_init(struct pocket_platform *p) {
  memset(p, 0, sizeof *p);
  const char *platform = getenv("PLATFORM"), *system = getenv("SYSTEM_PATH");
  if (!platform || strcmp(platform, "tg5040") || !system || system[0] != '/')
    return;
  if (snprintf(p->suspendHelper, sizeof p->suspendHelper, "%s/bin/suspend",
               system) >= (int)sizeof p->suspendHelper)
    return;
  p->active = true;
  for (int i = 0; i < 32; i++) {
    char path[64];
    snprintf(path, sizeof path, "/dev/input/event%d", i);
    int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
      continue;
    unsigned char keys[(KEY_MAX + 8) / 8] = {0};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof keys), keys) >= 0 &&
        (keys[KEY_POWER / 8] & (1u << (KEY_POWER % 8))))
      p->fds[p->count++] = fd;
    else
      close(fd);
  }
  fprintf(stderr, "NextUI power: %d evdev sources; suspend helper %s\n",
          p->count,
          access(p->suspendHelper, X_OK) == 0 ? "available"
                                              : "missing (no simulated sleep)");
}
void pocket_platform_close(struct pocket_platform *p) {
  for (int i = 0; i < p->count; i++)
    close(p->fds[i]);
  p->count = 0;
  p->active = false;
}
void pocket_platform_log_energy(const struct pocket_platform *p, const char *phase) {
  if (!p->active)
    return;
  static const char *paths[] = {
      "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor",
      "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq",
      "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq",
      "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq",
      "/sys/class/power_supply/axp2202-battery/capacity"};
  static const char *labels[] = {"governor", "cpu-khz", "min-khz", "max-khz", "battery-percent"};
  fprintf(stderr, "NextUI energy %s:", phase);
  for (unsigned i = 0; i < sizeof paths / sizeof *paths; i++) {
    FILE *f = fopen(paths[i], "r");
    if (!f)
      continue;
    char value[80];
    if (fgets(value, sizeof value, f)) {
      value[strcspn(value, "\r\n")] = 0;
      fprintf(stderr, " %s=%s", labels[i], value);
    }
    fclose(f);
  }
  fputc('\n', stderr);
}
void pocket_platform_key(struct pocket_platform *p, bool down, uint32_t now) {
  if (!p->active || (p->ignoreUntil && (int32_t)(now - p->ignoreUntil) < 0))
    return;
  if (down && !p->powerDown) {
    p->powerDown = true;
    p->pressedAt = now;
    fprintf(stderr, "NextUI power: button pressed; gameplay pause requested\n");
  } else if (!down && p->powerDown) {
    if (now - p->pressedAt >= 1000)
      p->shutdownRequested = true;
    else
      p->sleepRequested = true;
    fprintf(stderr, "NextUI power: %s requested\n",
            p->shutdownRequested ? "shutdown" : "sleep");
    p->powerDown = false;
  }
}
void pocket_platform_poll(struct pocket_platform *p, uint32_t now) {
  if (!p->active)
    return;
  for (int i = 0; i < p->count; i++) {
    struct input_event e;
    while (read(p->fds[i], &e, sizeof e) == sizeof e) {
      if (e.type == EV_SYN && e.code == SYN_DROPPED) {
        p->dropped[i] = true;
        p->powerDown = false;
        continue;
      }
      // Linux input protocol requires discarding the incomplete packet up
      // to SYN_REPORT before querying current key state. Old KEY_POWER
      // releases in that packet must not become a sleep request.
      if (p->dropped[i]) {
        if (e.type != EV_SYN || e.code != SYN_REPORT)
          continue;
        p->dropped[i] = false;
        unsigned char keys[(KEY_MAX + 8) / 8] = {0};
        if (ioctl(p->fds[i], EVIOCGKEY(sizeof keys), keys) >= 0)
          pocket_platform_key(
              p, (keys[KEY_POWER / 8] & (1u << (KEY_POWER % 8))) != 0, now);
        else
          p->powerDown =
              false; // No trustworthy held key; never infer shutdown.
        continue;
      }
      if (e.type == EV_KEY && e.code == KEY_POWER && e.value != 2)
        pocket_platform_key(p, e.value != 0, now);
    }
  }
  if (p->powerDown && !p->shutdownRequested && now - p->pressedAt >= 1000) {
    p->shutdownRequested = true;
    fprintf(stderr, "NextUI power: held button; shutdown requested\n");
  }
}
void pocket_platform_woke(struct pocket_platform *p, uint32_t now) {
  p->powerDown = p->sleepRequested = false;
  p->ignoreUntil = now + 1000;
  fprintf(stderr, "NextUI power: suspend returned; input debounce active\n");
  for (int i = 0; i < p->count; i++) {
    p->dropped[i] = false;
    struct input_event e;
    while (read(p->fds[i], &e, sizeof e) == sizeof e) {
    }
  }
}
bool pocket_platform_suspend(struct pocket_platform *p) {
  if (!p->active || access(p->suspendHelper, X_OK)) {
    fprintf(stderr, "NextUI power: real suspend helper unavailable\n");
    return false;
  }
  pid_t child = fork();
  if (child < 0)
    return false;
  if (!child) {
    execl(p->suspendHelper, p->suspendHelper, (char *)NULL);
    _exit(127);
  }
  int status;
  while (waitpid(child, &status, 0) < 0)
    if (errno != EINTR)
      return false;
  bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
  fprintf(stderr, "NextUI power: suspend helper %s (wait status %d)\n",
          ok ? "completed" : "failed", status);
  return ok;
}
bool pocket_platform_poweroff(struct pocket_platform *p) {
  if (!p->active || !p->shutdownRequested)
    return true;
  int fd =
      open("/tmp/poweroff", O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0)
    return false;
  bool ok = fsync(fd) == 0;
  if (close(fd))
    ok = false;
  return ok;
}
