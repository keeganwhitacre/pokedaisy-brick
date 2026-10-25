# Emerald expansion implementation candidate

This branch connects the existing upstream pk_wide.c GBA renderer, without
copying its ROM addresses or changing the working GB renderer path.
ROM recognition uses upstream's generated pk_wide_games.h CRC table.
Unknown ROMs retain the original viewport.

The host packs mGBA's 256-stride frame into the renderer's expected 240x160
input. EXPAND requests 256x192, filling the Brick at integer 4x with 8 more
world pixels on each side and 16 above/below. ZOOM requests 320x240.
The original source pixels remain at their native aspect with integer scaling.
Menus, battles, faults and unmatched scenes use the original image.
AUTO ZOOM IN gates text scenes through the existing scene classifier.
New GBA preferences default AUTO ZOOM IN to ON, matching Android. Existing
saved preferences and GB defaults are retained. Upstream classifies the START
overlay as TEXT, so disabling auto zoom intentionally keeps that overlay wide.
Original viewport equality is asserted by POCKET_TEST builds.

The prior battle-surroundings fade remains GB-only. GBA camera tweening,
HUD/minimap, item actions, other missing Android features and menu artwork
are separate follow-up work. This is not a completed Android parity claim.

Native Linux and actual NextUI ARM Release builds pass. The host functional
checks, shared profile checks and diagnostic GB/GBA runtimes pass natively and
under ARM/QEMU. The diagnostic runtime covers gameplay, save/state round trips,
companion navigation and original GBA scaling; it does not validate expanded
Emerald terrain. The inspected companion capture is explicitly a fixture.

Before a release, a matching retail Emerald ROM must verify outdoors, connected
maps, dialogue, menus, battle transitions, state loads and original-pixel
equality. No actual Emerald ROM or matching runtime state was supplied for
the initial implementation run. Device performance and appearance require testing.

## Private-ROM verification

The subsequently supplied retail English ROM matches SHA1
f3ae088181bf583e55daf962a92bb46f4f1d07b7. An isolated copy of the supplied
128 KiB save boots on Route 117 with a three-member party. The runtime harness
uses ordinary inputs, checks every original viewport pixel on matched frames,
captures original/expanded/zoom views and walking, and checks a state roundtrip.
It tests START fallback with AUTO ZOOM IN ON and captures real Party, Summary,
Bag descriptions, Map, Dex and Settings. Uploaded ROM/save files are unchanged;
they and the captures are never committed. The downloaded save's filename is
not evidence that its contents were earned through ordinary gameplay.
