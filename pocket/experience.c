/* GPL-3.0. SnapshotView.expProgress / ExpProgress fraction and toNext rules. */
#include "experience.h"
#include "experience_tables.h"
bool pocket_exp_progress(int growth, int level, uint32_t total, int width,
                         struct pocket_exp *out) {
  if (growth < 0 || growth >= 6 || level < 1 || level > 100 || width < 0 ||
      !out)
    return false;
  uint32_t start = experienceTables[growth][level],
           next = level == 100 ? start : experienceTables[growth][level + 1];
  uint32_t have = total > start ? total - start : 0, span = next - start;
  if (have > span)
    have = span;
  *out = (struct pocket_exp){
      start, next, total < next ? next - total : 0,
      span ? (int)((uint64_t)have * (unsigned)width / span) : width};
  return true;
}
