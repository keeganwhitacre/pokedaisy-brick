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
  unsigned item, quantity, pocket, slot;
  unsigned flyMenu;
  unsigned hold, heldKey;
  uint8_t partyRecord[64];
};
/* Supported only for verified normal Polished Crystal 3.2.3.
 * Opens the original Bag (target 2), or locates a unique ordinary/key item and
 * hands off at its verified game-owned action menu. Never chooses Use/Register/Give/Toss. */
bool pocket_action_begin(struct pocket_action *, const char *sha,
                         pocket_action_read, void *ctx);
/* Find a unique ordinary or key item and open its original action menu.
 * No selection is made inside that menu; the game owns legality and targeting. */
bool pocket_action_begin_item(struct pocket_action *, const char *sha, unsigned item,
                              pocket_action_read, void *ctx);
/* Fly uses the original party submenu and destination map. No destination,
 * badge, move, cursor or party writes; the user selects the actual flight. */
int pocket_action_flyer(const char *sha, pocket_action_read, void *ctx);
const char *pocket_action_fly_reason(const char *sha, pocket_action_read, void *ctx);
bool pocket_action_begin_fly(struct pocket_action *,const char *sha,
                            const uint8_t *rom,size_t romSize,pocket_action_read,void *ctx);
unsigned pocket_action_tick(struct pocket_action *, pocket_action_read,
                            void *ctx);
void pocket_action_cancel(struct pocket_action *);
const char *pocket_action_message(const struct pocket_action *);
#endif
