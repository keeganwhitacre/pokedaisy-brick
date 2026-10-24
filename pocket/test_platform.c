/* GPL-3.0. Linux input packet tests; no real suspend/shutdown execution. */
#define _POSIX_C_SOURCE 200809L
#include "platform_nextui.h"
#include <assert.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>
static void event(int fd, unsigned type, unsigned code, int value) {
  struct input_event e = {.type = type, .code = code, .value = value};
  assert(write(fd, &e, sizeof e) == sizeof e);
}
int main(void) {
  int fds[2];
  assert(pipe(fds) == 0);
  assert(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0);
  struct pocket_platform p = {.active = true, .count = 1, .fds = {fds[0]}};
  event(fds[1], EV_KEY, KEY_POWER, 1);
  pocket_platform_poll(&p, 100);
  assert(p.powerDown && !p.sleepRequested && !p.shutdownRequested);
  event(fds[1], EV_KEY, KEY_POWER, 2); // autorepeat cannot reset hold timer
  pocket_platform_poll(&p, 500);
  assert(p.pressedAt == 100);
  event(fds[1], EV_KEY, KEY_POWER, 0);
  pocket_platform_poll(&p, 600);
  assert(!p.powerDown && p.sleepRequested);
  pocket_platform_woke(&p, 700);
  event(fds[1], EV_KEY, KEY_POWER, 1);
  pocket_platform_poll(&p, 800);
  assert(!p.powerDown && !p.sleepRequested);
  event(fds[1], EV_KEY, KEY_POWER, 1);
  pocket_platform_poll(&p, 1800);
  assert(p.powerDown);
  event(fds[1], EV_SYN, SYN_DROPPED, 0);
  event(fds[1], EV_KEY, KEY_POWER, 0); // stale incomplete packet
  pocket_platform_poll(&p, 1900);
  assert(p.dropped[0] && !p.sleepRequested);
  pocket_platform_poll(&p, 2900);
  assert(!p.shutdownRequested); // a lost release cannot become a long hold
  event(fds[1], EV_SYN, SYN_REPORT, 0);
  pocket_platform_poll(&p, 2950); // pipe ioctl fails: no inferred held press
  assert(!p.dropped[0] && !p.powerDown && !p.sleepRequested);
  event(fds[1], EV_KEY, KEY_POWER, 1);
  pocket_platform_poll(&p, 3000);
  pocket_platform_poll(&p, 4000);
  assert(p.shutdownRequested && !p.sleepRequested);
  pocket_platform_close(&p);
  close(fds[1]);
  puts("PASS power packets: short press, hold, autorepeat, wake debounce, "
       "SYN_DROPPED discard and failed-resync safety; no hardware sleep "
       "invoked");
}
