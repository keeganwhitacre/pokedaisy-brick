/* The QUICK MENU (the Y button): the app's QuickMenu.kt in RING / GRID styles and the GLASS look, drawn into
 * a canvas whose see-through pixels carry their alpha (the host lays it over the game). GPL-3.0. */
#ifndef POCKET_QUICK_MENU_H
#define POCKET_QUICK_MENU_H
#include "pd_canvas.h"
#include "pd_game.h"
#include <stdbool.h>

enum quick_action { QUICK_PC, QUICK_FLY, QUICK_REPEL, QUICK_BIKE, QUICK_WILD, QUICK_SAVE_STATE, QUICK_LOAD_STATE, QUICK_FF, QUICK_COMPANION, QUICK_COUNT };

struct quick_item {
  enum quick_action action;
  bool off;          // WILD ENCOUNTERS switched off: an OFF tag (QuickTile's)
  bool lit;          // FF running
  const char *why;   // why it can't run now (dimmed), or NULL
  int species;       // FLY's icon: the Pokémon that would carry the player (0 = none)
  int item;          // REPEL's / BIKE's icon: the bag's own item (0 = none)
  const char *label; // in place of the action's name (BIKE's GET OFF while riding), or NULL
};

/* What a canvas pixel holds where nothing was drawn (fully see-through). */
#define QUICK_CLEAR 0x00FF00FFu

/* Draws the selected layout (n tiles, sel highlighted) and its hint over QUICK_CLEAR; returns false if it
 * doesn't fit the canvas. polishedBuild is the matched ROM-art profile, or -1. */
bool quick_draw(struct pd_canvas *c, const struct pd_game *g, const struct quick_item *items, int n, int sel,
                int polishedBuild, bool grid);
/* FLY's places / REPEL's kinds (QuickMenu.kt's PickList): a titled list over QUICK_CLEAR, row sel highlighted,
 * scrolled to keep it in view. */
void quick_list_draw(struct pd_canvas *c, const char *title, const char *const *rows, int n, int sel);
/* The tile a D-pad press (dx, dy: -1 / 0 / 1) moves to from sel: the nearest one that way round the ring. */
int quick_step(int n, int sel, int dx, int dy, int w, int h);
/* GRID uses the same centred, balanced rows for drawing and navigation. */
int quick_step_layout(int n, int sel, int dx, int dy, int w, int h, bool grid);
const char *quick_label(enum quick_action a);
/* After drawing: every pixel opaque but the clear ones and the glass's own alpha. */
void quick_finish(struct pd_canvas *c);
#endif
