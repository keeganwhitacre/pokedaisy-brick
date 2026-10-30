#ifndef POCKET_POLISHED_H
#define POCKET_POLISHED_H
#include "battle_action.h"
#include "favorites.h"
#include "pd_canvas.h"
#include "pd_snapshot.h"
void pc_font(const unsigned char *, size_t, int);
int pc_identify(const unsigned char *, size_t);
bool pc_read(struct pd_snapshot *, pd_read_fn, void *);
void pc_draw(struct pd_canvas *, const struct pd_snapshot *, int selected,
             bool summary, const unsigned char *, size_t, int build);
void pc_bag_draw(struct pd_canvas *, const struct pd_snapshot *, int pocket,
                 int selected, int build);
enum pc_section {
  PC_PARTY,
  PC_BAG,
  PC_BATTLE,
  PC_DEX,
  PC_MAP,
  PC_GUIDE,
  PC_SECTIONS
};
int pc_dex_count(void);
void pc_battle_draw(struct pd_canvas *, const struct pd_snapshot *,
                    const unsigned char *, size_t, int);
void pc_dex_draw(struct pd_canvas *, const struct pd_snapshot *, int selected,
                 bool detail, const unsigned char *, size_t, int);
void pc_item_draw(struct pd_canvas *, int item, const unsigned char *, size_t,
                  int);
// Game-specific navigation stays out of the emulator/SDL host.
struct pc_ui {
  int section, selected, pocket, item, dex, dexPage, region, mapX, mapY, place,
      move, summaryPage, typePage, guidePage, guideRow, guideReveal,
      guideScroll, battlePage, battleRow, battleFoe, battleType, dexFilter,
      bagSort, bagScroll, bagDescPage, bagKeyPocket, bagKeySlot, dexScroll;
  unsigned battleSeen;
  uint64_t guideOpenKey;
  int guideRowScroll, guideRowScrollMax, guideBuild, guideOpenPage;
  bool guidePagesFocused, guideFollowSelection;
  uint32_t animationMs;
  bool wasBattle, bagStarted, bagReselect;
  bool summary, detail, mapSet, placesOpen, moveOpen;
  bool battleControls, battleParty, battleSupported;
  struct pocket_favorites favorites;
  bool favoritesDirty;
  int bagBuild;
  int battleChoice, battleRequest, battleValue;
  struct pocket_battle_menu battleMenu;
};
enum pc_action {
  PC_UP,
  PC_DOWN,
  PC_LEFT,
  PC_RIGHT,
  PC_CONFIRM,
  PC_BACK,
  PC_PREVIOUS,
  PC_NEXT,
  PC_ALTERNATE,
  PC_PLACES,
  PC_FAVORITE
};
void pc_ui_tick(struct pc_ui *, uint32_t);
/* Changes only when an actually visible discrete animation changes frame. */
unsigned pc_ui_animation_key(const struct pc_ui *);
void pc_ui_observe(struct pc_ui *, const struct pd_snapshot *);
bool pc_ui_input(struct pc_ui *, const struct pd_snapshot *, enum pc_action);
void pc_ui_draw(struct pc_ui *, struct pd_canvas *, const struct pd_snapshot *,
                const unsigned char *, size_t, int);
int pc_lz(const unsigned char *, size_t, size_t, unsigned char *, size_t);

void pc_hud_icon(struct pd_canvas *, int, int, int, const unsigned char *, size_t, int);
#endif
