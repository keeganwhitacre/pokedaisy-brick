# PokeDaisy Pocket: Phase 7

Based on PokeDaisy by Lidor Itzhari - https://github.com/lidor30/pokedaisy

Native SDL2 and mGBA prototype for TrimUI Brick / NextUI. The user has confirmed
Phase 1 gameplay and a live party with a ROM sprite on physical Brick hardware.
Phase 7 adds the L3 quick-menu ring with working state, fast-forward and companion actions. The previous pass added battle effectiveness, Suggestions, Foe Details and Foe Team browsing. The previous pass added the Polished Crystal Guide: live encounters, area finds, progress-aware next bosses, tips and Where Is. It retains the original ROM font, all three region maps and two-column summaries. It has been tested using the supplied supported 3.2.3 ROM. It contains a real emulator and companion integration.
It does not yet reproduce the full Android application.

Gameplay preserves the working opaque RGB888 format. Game scaling now uses the
physical panel resolution independently of the companion's logical UI grid.
The original display hotfix remains included. Supported Polished Crystal 3.2.x
builds now offer Party/Summary, Bag, Battle, Pokedex, Map and Guide; 3.1.1 remains emulation-only.

## Install

1. Copy `Tools/tg5040/PokeDaisy.pak` from the delivery archive into the same
   location on your SD card. Keep the whole folder together.
2. Put your own uncompressed `.gb`, `.gbc`, or `.gba` ROMs inside the pak's
   `roms` folder. No ROMs are included in the install package.
3. Open Tools > PokeDaisy in NextUI. Choose a game with the D-pad and A.
4. Press X during gameplay to open the companion. B or X returns to gameplay.

The companion draws at 512 x 384 with crisp 2x scaling to 1024 x 768.
Gameplay uses the largest whole-pixel scale that fits the physical display:
Game Boy / Color is 5x (800 x 720), centered with small borders; GBA is 4x
(960 x 640). This increases GB image width and height by 25% over Phase 1.
The original aspect ratio is preserved, with no stretching, filtering or crop.
No Android runtime, touchscreen or mouse is used.

When updating an existing installation, merge the updated files into the pak
and keep your `roms`, `saves` and `states` folders.

## Controls

| Button | Gameplay | Companion / menus |
| --- | --- | --- |
| D-pad | Game directions | Select a party member or move focus |
| A / B | Game A / B | Confirm / back |
| Start / Select | Game Start / Select | Normal menu navigation uses A/B |
| X | Open companion | Return to game |
| L3 | Quick-menu ring | Open / close quick menu |
| R3 | Reserved for future zoom | No zoom action yet |
| Y | Toggle fast-forward (2x default) | Screen-specific alternate action |
| L1 / R1 | Game L / R on GBA | Previous / next section in either companion |
| Menu | Open game menu | Open / close game menu |
| Select + Start held together | Open game menu | Open / close game menu |
| L2 / R2, while paused | No action during gameplay | Confirm save / load state |

The game menu also offers resume, party, state saving/loading, fast-forward,
quit and About. Loading a state and quitting require confirmation. Save inside
the game before choosing Quit; quitting does not perform the game's save action.
In Polished Crystal, L/R cycles Party, Bag, Battle, Dex and Map. In Party, the D-pad
follows the two-column grid and A opens a summary. Within a summary, Up/Down changes Pokemon, Left/Right selects a move, A opens its ROM description, Y cycles Info/Weaknesses/Resistances/Immunities, and Select pages long matchup lists. B returns one level. Eggs conceal their moves. In Bag, Left/Right changes
pocket and Up/Down scrolls through items. The Bag shows live quantities and
money, including medicine grouped under Items, plus Balls, TMs/HMs, Berries and
Key items. Press A for an item description read from the matching ROM. The Bag remains read-only; upstream does not provide Polished item-use actions.
Battle uses Y to cycle INFO, SUGGESTIONS, FOE DETAILS and FOE TEAM. INFO rates damaging moves against the selected foe. Suggestions uses upstream’s power/type/defensive-risk heuristic, not a full damage calculation. Foe Details shows actual active-battler stats, held item and moves; reserve stats are omitted because the Polished party reader does not provide them. On Foe Team use D-pad and A to inspect a member. Unannounced slots remain unnamed until selected; Select follows the live foe again. B returns to INFO, then gameplay. No battle commands or memory writes are introduced.
Dex shows live seen/caught flags in Polished Crystal's 289-entry order. Up/Down chooses, Left/Right pages and A opens an entry. Up/Down browses entries and B returns to the list. Entry sprites, category, text, size and base stats read from the matching ROM. Y switches to the full-text/abilities page.
Map displays Johto, Kanto and Orange with original ROM palettes, tile flips and player art. The D-pad moves its cursor; A changes region, Y recenters on the player, and Select opens Places. A on a place shows it on the map. Map is read-only; Fly is not implemented.
Guide uses Left/Right to switch HERE: WILD, HERE: FINDS, NEXT BOSS, TIPS and WHERE IS. Up/Down chooses a row or scrolls an open answer. A reveals a hint, then an answer; B closes it. Y changes encounter method or Where Is category. A plus mark means caught/collected. The Guide follows the game clock and badges; it never changes game data. Upstream marks its guide data unverified, so entries may contain mistakes.
D-pad and A activate the existing GBA controls through their recorded hit regions.
Long lists use repeated directions to move and scroll. Maps need further controller
work; this build does not promise complete access to every point on a map.

Desktop keys: Z=A, X=B, Enter=Start, Backspace=Select, Tab=companion,
F=fast-forward, Q/E=L/R, Escape=Menu, 1/2=state confirmations while paused.

## Game support

- GB/GBC/GBA games run through the pinned upstream mGBA 0.10.5 core.
- Polished Crystal: the 16 SHA-1 matched normal, faithful and debug builds of
  3.2.0 through 3.2.3 listed by upstream. Read-only party decoding includes
  9-bit species, variant forms, level, HP, status, gender, moves and PP.
  Summary includes species and move types, held item and EXP, with faithful-build overrides.
  Held-item markers are extracted from the matching user ROM.
  The Bag reads all five companion pockets with validated counts and terminators.
  Icons are extracted from the matching user's ROM using upstream's mini-icon
  table and LZ format. The user confirmed a ROM sprite on Brick; the local fixture tests do not exercise ROM icon extraction.
- FireRed/LeafGreen and Emerald: upstream's portable C companion supports
  party, summary, bag, battle, map, dex, guide and trainer-card features where
  the existing version-specific tables permit them. These screens are reused,
  not replaced by mock screens. Tabs requiring ROM tables cannot work fully
  without a supported real ROM. See upstream `3ds/README.md` for exact coverage.
- Other games can run, but their companion reports unsupported. There is no DS,
  RetroAchievements, networking or ROM modification in this frontend.

## Saves and existing progress

Data stays inside the pak:

- `saves/<complete ROM filename>.sav` for in-game saves, for example
  `saves/MyGame.gbc.sav`.
- `states/<complete ROM filename>.ss` for one save-state slot.
- `settings.ini` for GBA companion settings.
- `launch.log` for errors.

Existing NextUI/RetroArch saves are never discovered, moved or overwritten.
To carry over progress, first make a separate backup, then COPY the raw battery
save to the path above, matching the complete ROM filename. Do not copy a save
state from a different emulator or core build. No conversion or automatic import
is implemented.

Before opening an existing battery save, the app makes a uniquely named
`.backup-*` copy beside it. A failed backup prevents the game from opening.
Backups are not automatically pruned. Save states use a temporary file and rename.
State files include RTC but do not request SRAM restoration when loading, so an
old state cannot automatically replace the current battery save. Saving inside
the game after rewinding is still an explicit write to that save, as expected.
Clean quit and SIGINT/SIGTERM unwind mGBA normally. Power loss and forced SIGKILL
cannot be made equivalent to a clean shutdown; Brick suspend/power handling is
still a hardware verification item.

## Build from source

Complete corresponding source is published at https://github.com/keeganwhitacre/pokedaisy-brick.
Each install archive records its exact source commit and mGBA pin in Source/SOURCE.md.
Clone that commit and initialize only its mGBA submodule:

```
git submodule update --init third_party/mgba
cmake -S pocket -B pocket/build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build pocket/build-linux -j4
```

Linux build dependencies: C compiler, CMake >=3.18, SDL2 development headers,
POSIX threads, libm and libdl; zlib is optional if found by mGBA. No Qt, Android,
OpenGL frontend, DS core, achievement library or online services are built.

For Brick, prepare the pinned, checksum-verified NextUI toolchain and TrimUI SDK (Python 3.12+, curl and xz):

```
python3 pocket/scripts/setup_brick_sdk.py /absolute/path/to/brick-sdk
```

Set NEXTUI_TOOLCHAIN to the printed directory, then:

```
export NEXTUI_TOOLCHAIN=/absolute/path/to/x86_64-aarch64-nextui-linux-gnu
cmake -S pocket -B pocket/build-brick \
  -DCMAKE_TOOLCHAIN_FILE=nextui-toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build pocket/build-brick -j4
python3 pocket/package.py pocket/build-brick/pokedaisy-pocket /path/to/output
```

The binary is compiled for Cortex-A53, AArch64 Linux using GCC 8.3 and the target
SDK sysroot. `package.py` uses NextUI's system SDL2 and system C runtime; it does
not bundle or replace firmware libraries. The launcher's optional `lib` path is
reserved for future explicitly packaged dependencies. A missing system library
will be recorded in `launch.log`.

## Verification and limitations

Read `VERIFICATION.md` for exact results. The supplied supported Polished Crystal 3.2.3 ROM was tested through a fresh game without fixture injection. The older 3.1.1 test remains emulation-only.
The user has confirmed the Phase 1 gameplay/party/sprite path on a physical Brick.
The Phase 7 quick menu and physical L3 detection still need a hardware retest. The supplied supported 3.2.3 ROM passes all 289 dex texts/front pictures, 373 item/TM/key descriptions, all three maps, and a normal-input new game to the starter. The ARM frontend reads that actual party and passes pause/resume and state tests without fixture injection.
Captures prefixed `runtime-live323` use the actual new game and matching ROM art. Captures named `fixture` use upstream captured memory. See the verification report for each capture's scope. The install binary
contains no fixture injection and requires a supported game for real party data.
The Polished Crystal layout uses the Android palette, six-slot party structure, ROM icons and the ROM font at crisp integer scaling. Summary uses Android's information/moves hierarchy. Remaining battle details/styling, list controls, multi-slot state browsing and exact screen/animation parity remain unfinished. Android’s Polished path does not currently provide IV/EV summary, LEARN/evolution tables or FieldItems actions. Read `PARITY.md` for the feature-by-feature boundary.

For upstream updates and the generated-definition workflow, read `UPSTREAM.md`.

## License

GPL-3.0, with upstream's attribution requirement preserved in `NOTICE` and About.
mGBA is MPL-2.0, blip_buf LGPL-2.1, inih BSD, and Pixel Operator CC0. The complete
corresponding application source and pinned mGBA source are freely available at the
repository and commit recorded in Source/SOURCE.md beside the install package. See
`LICENSE`, `NOTICE` and the upstream license files. No commercial ROMs or extracted
game art are distributed.

L3 now opens the quick menu over paused gameplay. D-pad directions select the four actions; A runs one, B or L3 closes it. Save/load open the existing confirmation; loading is disabled until a state exists. Menu > Quick menu is also available. The original game image keeps its physical integer scale behind the overlay.

R3 remains reserved for zoom. Zoom and expanded overworld rendering are not in this build. Upstream supports retail Crystal expansion, but Polished requires its own verified definitions.

L3/R3 are discovered from SDL controller bindings or Linux joystick/evdev button codes. The launch log records the result. If a driver does not identify them, set `POCKET_L3_BUTTON` / `POCKET_R3_BUTTON` to its raw SDL button numbers (0–255) in launch.sh, or use Menu > Quick menu. No unverified raw index is assumed for these buttons. Keyboard F3 is the quick-menu shortcut for development.
