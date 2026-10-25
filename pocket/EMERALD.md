# Emerald expansion implementation candidate

This branch connects the existing upstream pk_wide.c GBA renderer, without
copying its ROM addresses or changing the working GB renderer path.
ROM recognition uses upstream's generated pk_wide_games.h CRC table.
Unknown ROMs retain the original viewport.

The host packs mGBA's 256-stride frame into the renderer's expected 240x160
input. EXPAND requests 240x180, keeping the original horizontal field and
adding vertical world for the Brick's 4:3 shape. ZOOM requests 320x240.
The original source pixels remain at their native aspect with integer scaling.
Menus, battles, faults and unmatched scenes use the original image.
AUTO ZOOM IN gates text scenes through the existing scene classifier.
Original viewport equality is asserted by POCKET_TEST builds.

The prior battle-surroundings fade remains GB-only. GBA camera tweening,
HUD/minimap, item actions, other missing Android features and menu artwork
are separate follow-up work. This is not a completed Android parity claim.

Before a release: native/NextUI ARM builds and diagnostic regressions must
pass, then a matching retail Emerald ROM must verify outdoors, connected
maps, dialogue, menus, battle transitions, state loads and original-pixel
equality. No actual Emerald ROM or matching runtime state was supplied for
this implementation run. Device performance and appearance require testing.
