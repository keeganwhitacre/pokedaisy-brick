# PokeDaisy as a plorpOS core (v0.3 work)

`pokedaisy_libretro.so` is mGBA's own libretro core with the companion layered over it. diatom, the resident
libretro frontend in plorpOS, runs it like every other core, and owns what a frontend owns: save states, resume,
fast-forward, rewind, shaders, achievements, audio and the game switcher. This page is the parity table against the
v0.2 SDL host (`docs/PLORPOS_V0.2.md`) and the list of intentional differences.

## How it is built

- `libretro/` holds the layer (`pd_libretro.c`), the CMake file, the export list (`retro.map`) and a desktop test
  frontend (`test/pd_lr_host.c`).
- mGBA's `src/platform/libretro/libretro.c` is compiled unchanged: `pd_libretro.c` includes it with its entry points
  renamed by the preprocessor, then defines the real `retro_*` functions around them.
- Only `retro_*` is exported (`test/check_exports.sh`); a second mGBA is resident beside the stock core.
- Output is RGB565. The core picks its own frame size from the panel (`pokedaisy_panel`, sent by plorpOS; `auto` = the older `pokedaisy_canvas` option alone, 320x240 by default)
  and the aspect: the game's whole scale `s` is the largest that fits the panel, and the frame is the panel divided by
  `s`. EXPAND (and any game without EXPAND support) uses the full scale, 4x on the Brick = 256x192; ZOOM OUT uses one
  scale less, 3x = 341x256. ORIGINAL uses the ZOOM OUT frame (a Game Boy game 4x on the Brick, 256x192). The companion
  and the quick menu use the menu frame while they're open: the ZOOM OUT frame, or a scale less again until it holds 320x240 (what the companion is laid out for: a Game Boy game's ZOOM OUT
  on the Brick is 256x192, so its menus get 341x256), the same in every aspect. The HUD sits on the picture that is on
  screen: in EXPAND and ZOOM OUT its corners are the whole frame's, not the game's middle. The HUD cursor keeps the
  frame that is on screen (it is not a menu: it sits on the game's own view). A change of frame is a
  `SET_GEOMETRY`; diatom re-fits the picture.
  The less the frame, the less the core draws: EXPAND at 4x costs about 8.5 ms a frame on the Brick, ZOOM OUT about 11.
  AUTO ZOOM switches between the two as the scene changes (field = ZOOM OUT frame, elsewhere the EXPAND frame).
- Buttons: the quick menu (Y), companion (X) and HUD cursor (R2) buttons are rows of PokeDaisy's own SETTINGS, saved in `settings.ini` (a core option is out of reach in plorpOS). The launcher keeps L2 and R2 for this core on GBA (it clears them for the other GBA cores).
- Off the map (menus, fast-forward, rewind) the window past the frame is the ambient glow, blurred from the frame's own edges, for GBA and for Game Boy games. Fast-forward skips the map rebuild and draws only the glow (`pk_wide_glow`) and plays no audio. Rewind (`RETRO_THROTTLE_REWINDING`) holds the chosen aspect: AUTO ZOOM IN does not step in on rewound frames.
- The companion's D-pad cursor in a list (BAG, DEX, GUIDE, SETTINGS, PLACES) moves one row a press. A row cut by the
  list's edge scrolls wholly into view when the cursor lands on it, and past the last row shown the list scrolls one row
  and the cursor goes onto the new row. The cursor leaves the list (for the tab bar) only once the list is at its end.
- Game Boy games' BAG: the closed pocket tabs are ink chips with white text (the open one is black on white).
- Settings: `<save directory>/pokedaisy/settings.ini`.

## Intentional differences from v0.2

| Difference | Why |
|---|---|
| No SAVE STATE, LOAD STATE, FAST-FORWARD or LEAVE GAME in the quick menu or SETTINGS | diatom owns them (`frontendOwnsPlayback`) |
| No game menu (resume, states, quit) | the launcher's in-game menu is that |
| The quick menu pauses the game | brief; FLY's ask runs frames while it waits for a safe moment |
| The game is drawn at 1x of its frame, which is 4x (EXPAND) or 3x (ZOOM OUT) on the glass | the core picks the frame from the aspect |
| ZOOM OUT is one whole scale below EXPAND (3x against 4x on the Brick), not a different map size on one canvas | what v0.2's ZOOM OUT does; the frame is what changes |
| SCREEN, FILTER (LCD), scaling, rewind and fast-forward settings are not offered | diatom's shaders, display modes and hotkeys |
| The HUD hotkey is a core option button (R2), not a hotkey-file entry | the launcher drops the `hud` action: diatom refuses a hotkey list that names it |
| mGBA's Super Game Boy border is forced off | EXPAND needs the 160x144 frame (the SDL host does the same) |
| The pixels pk_wide reads are 565 expanded with alpha 0xFF | the 32-bit layer flags of the SDL build do not exist at 16 bits (see unchecked rows) |

## Parity table

"Checked how": D = Brick, retail FireRed with a `/tmp` copy of the owner's save, through the launcher or standalone
diatom; H = the desktop test frontend with the same ROM and save copy; P = shared `3ds/core` code with the v0.2
preview byte-identical (80 PNGs); NC = not checked.

| Feature (v0.2) | Core location | Result | Checked how |
|---|---|---|---|
| Companion tabs PARTY, BAG, MAP, GUIDE, SETTINGS | `pd_ui_draw` | same on FireRed | H: each tab shown with the owner's team, bag, map and GUIDE > HERE; D: PARTY through the launcher |
| DEX, CARD on retail FireRed | `3ds/core` | "NO POKéDEX YET" on this rev 0 cartridge, as v0.2 documents | H |
| The same tabs on Emerald, Emerald Legacy, Unbound, Crystal, Blue | `pd_ui_draw` | PARTY, BAG, MAP, GUIDE draw with each save's own team and bag, in ORIGINAL, EXPAND and ZOOM OUT (Crystal and Blue: MAP is the game's own town map since the Game Boy MAP work, see the next row; Unbound GUIDE says its guide is not checked in-game yet). Polished Crystal NC (no save). Not compared frame by frame with v0.2 | H: the owner's saves from the Brick card, 1024x768 panel |
| Game Boy MAP (Red, Blue, Yellow; Crystal rev 0 / rev 1 / AU; Gold, Silver) | `pd_gb_map.c`, `pd_map.c` | the game's own town map rebuilt from the cart (Gen 1: one KANTO page; Gen 2: the POKéGEAR's JOHTO and KANTO), the player's head (Crystal: CHRIS or KRIS), the place name in black on a white strip over the map's top row, a black cursor, PLACES, a tap or the D-pad cursor picking the smallest place under it; a place off the map (FAST SHIP) shows its name only. Gold / Silver now name their place too. Not on PureRed / PureBlue, the European Yellows or the Crystal hacks | P: GB fixtures (Red, Blue, Yellow, Crystal, Gold, Silver) with their carts; H: Crystal save, 1024x768 panel |
| Summary, STATS, INFO, shiny marks, icons | `3ds/core` | NC in the core; INFO and STATS also on 240-row frames (720x480 panels, the 3DS), with the name and HP lines packed closer there; STATS has one line per stat and HIDDEN POWER on one line | P; desktop host 720x480, 1024x768, 1600x1440 (Emerald Legacy) |
| HUD parts | B2 | same as v0.2 (balls, BAG, MAP, COMPANION; PC after B3) | D: G2, core and v0.2 side by side |
| HUD hotkey cursor | B2 | opens on R2 and on L2 (the setting row); it did nothing on GBA until the launcher stopped clearing L2/R2 for this core | D, owner on the Brick |
| Quick menu PC | B3 | works | D |
| Quick menu FLY | B3 | list of visited towns, flight to Pallet Town; indoors "You can't FLY from here." | D |
| Quick menu REPEL | B3 | dimmed "No REPEL in your bag"; use NC (the save has none) | D |
| Quick menu BIKE | B3 | on and off | D |
| Quick menu WILD ENCOUNTERS | B3 | OFF tag, notice, persisted; no battle in 96 grass steps, a battle with it on. Not shown for a game without the switch | D, with the control |
| Game Boy quick menu: PC, FLY, WILD ENCOUNTERS (Red, Blue, Yellow, Crystal rev 0 / rev 1 / AU; PureRed / PureBlue 2.7.6) | `linux/gb_field.c` (the app's GbField) | each the game's own way in: Gen 1's PC routine called from its overworld loop, its FLY the town map's warp flags, its encounters the map's rates held at 0; Crystal's PC and FLY queued as map scripts, its encounters' cooldown held. A queued action the game hasn't taken within 24 running frames is withdrawn. Each cart's code is checked first (`gb_field_test`) | desktop host: Crystal PC opens, FLY (badge / Fly / spawns poked in) lists 12 Johto towns and lands in New Bark Town, WILD OFF holds the cooldown; Blue's PC from the field and back |
| Game Boy HUD (English Yellow, Crystal) | `linux/hud.c`, `pd_libretro.c` | where the field scene is known, as in the app: balls, BAG, PC, WILD (from the GUIDE), MAP, the party in ZOOM OUT; WINDOW is the game's black-on-white window; the compact HUD drops its last buttons / ball kinds where a narrow picture can't hold them | desktop host, Crystal in ORIGINAL, EXPAND, ZOOM OUT |
| Touch (RG Rotate) | diatom's Rotate port, `pd_libretro.c` | while PokeDaisy runs, the panel's first finger is the libretro pointer over the picture (no hold-for-Menu; the launcher's edge swipes held back as in the DS screens): taps and drags on the companion, a tap on a HUD part presses it, a tap on a quick menu tile or FLY / REPEL row picks it, outside closes. The game picture itself takes no touch | desktop host (`-t`): HUD BAG, a tab chip, the quick menu's PC, a tap outside |
| EXPAND, ORIGINAL on FireRed | B4 | same scene and window as v0.2 | D, Vermilion City, side by side |
| ZOOM OUT on FireRed | B4 | 341x256 frame (3x), against EXPAND's 256x192 (4x); the owner checked it on the Brick ("looks good") | H, D |
| EXPAND, ZOOM OUT on Crystal | B4 | the overworld widens past the game's edges (canvas 204x153 in EXPAND, 256x192 in ZOOM OUT, its menus 341x256); the border drew in the wrong colours until the 16-bit palette was converted (rebuilt window 360 of 360 cells differed from the frame before, 0 after); the Crystal clock prompt appears because the desktop host has no RTC | H, D (border colours, owner) |
| Fast-forward and rewind edges (GBA and Game Boy) | pk_wide | the picture over the blurred glow, not black or flat bars; rewind keeps ZOOM OUT; fast-forward is silent | H, D (owner) |
| AUTO ZOOM IN | B4 | steps ZOOM OUT in to the EXPAND frame on menus, the START menu, battles and text (Crystal start/select, owner); building interiors are the overworld, so nothing steps in there (by design, as v0.2); Yellow NC (the community save does not load as a game; follow-up bead) | D |
| GB look, Gen 1 DEX pages, GB GUIDE | `3ds/core` | Blue: DEX lists all 151 (SEEN 151, OWN 151); GB tabs and GUIDE notice draw on Blue and Crystal. SGB colours NC; Gen 2 DEX pages not opened | H (Blue, Crystal saves) |
| Place names, item descriptions, hack maps, guides, cards | `3ds/core` | place name and GUIDE HERE shown on FireRed; hacks NC | H, D |
| Save states, fast-forward, rewind | diatom | the in-game menu has Save, Load; state resume works; fast-forward measured at 1.8x on EXPAND (104 / 112 fps at 2x / 3x); rewind works (owner). FF audio discard untested on device | D |
| Battery save, resume slot | diatom | the save file is the one mGBA uses (checksum unchanged after a session without saving); resume restores the state, twice in a row | D |
| Game switcher | diatom | NC | |
| Achievements | diatom | NC | the device had no Wi-Fi and no cached set |
| Music over the game, Bluetooth audio | diatom | NC | |

## Gate G4

Not reached: the rows marked NC above are not checked, so the SDL host stays (unit E is not done). The launcher
path is: Core row, launch, companion, quick menu, quit, resume, twice in a row.
