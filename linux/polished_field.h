/* GPL-3.0. Version-verified Polished Crystal field-script bridge. */
#ifndef POCKET_POLISHED_FIELD_H
#define POCKET_POLISHED_FIELD_H
#include "game_action.h"
struct mCore;
bool polished_field_supported(const char *sha);
/* NULL means ready; otherwise a player-facing reason. Does not write. */
const char *polished_pc_reason(const char *sha, pocket_action_read, void *ctx);
/* Queue the game's existing PC script through the shared GB adapter.
 * No CPU injection, box edits, automatic confirmations or save writes. */
bool polished_pc_open(struct mCore *, const char *sha, pocket_action_read, void *ctx);
/* Tracks a requested PC session until the script and original menus finish. */
bool polished_pc_busy(pocket_action_read, void *ctx);
#endif
