/* GPL-3.0. Downstream, controller-only gameplay action dispatcher.
 * No write-memory interface. Game-specific observations stay outside SDL. */
#ifndef POCKET_GAME_ACTION_H
#define POCKET_GAME_ACTION_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef bool (*pocket_action_read)(void *, uint32_t, void *, size_t);
enum pocket_action_result {
  PA_IDLE,
  PA_RUNNING,
  PA_DONE,
  PA_REJECTED,
  PA_TIMEOUT,
  PA_CANCELLED
};
struct pocket_action {
  enum pocket_action_result result;
  unsigned frames, phase, pulse, stable;
  int target;
};
/* Supported only for the verified normal Polished Crystal 3.2.3 SHA-1.
 * Request opens the original Bag (target 2). Further actions are deliberately
 * unavailable until their normal-game completion predicates are verified. */
bool pocket_action_begin(struct pocket_action *, const char *sha,
                         pocket_action_read, void *ctx);
unsigned pocket_action_tick(struct pocket_action *, pocket_action_read,
                            void *ctx);
void pocket_action_cancel(struct pocket_action *);
const char *pocket_action_message(const struct pocket_action *);
#endif
