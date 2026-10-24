/* Linux host policies around upstream pk_savevf; no game-specific addresses. */
#ifndef POCKET_HOST_IO_H
#define POCKET_HOST_IO_H
#include <mgba-util/vfs.h>
struct pocket_save_status {
  bool failed, closed;
};
/* Takes inner on success or failure. status must outlive the returned VFile. */
struct VFile *pocket_save_wrap(struct VFile *inner,
                               struct pocket_save_status *status);
/* Preferences use content identity, not filenames, and contain no game data. */
int pocket_display_load(const char *root, const char *sha1);
bool pocket_display_save(const char *root, const char *sha1, int mode);
#endif
