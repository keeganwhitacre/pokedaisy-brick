/* Compatibility wrapper for the existing Polished Crystal adapter. */
#include "experience.h"
#include "../3ds/core/pd_experience.h"
bool pocket_exp_progress(int growth, int level, uint32_t total, int width,
                         struct pocket_exp* out) {
  struct pd_exp xp;
  if (!out || !pd_exp_progress(growth, level, total, width, &xp)) return false;
  *out = (struct pocket_exp){xp.start, xp.next, xp.toNext, xp.filled};
  return true;
}
