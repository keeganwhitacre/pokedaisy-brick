# PokeDaisy Pocket: Phase 16

Based on PokeDaisy by Lidor Itzhari - https://github.com/lidor30/pokedaisy

Native SDL2 and mGBA port for TrimUI Brick / NextUI. The user has confirmed
gameplay, expanded/zoomed overworlds, companion menus, saves and the quick menu
on physical hardware. Those working paths are retained.

Phase 16 keeps safe expanded conversations at the selected scale. AUTO ZOOM
IN restores upstream's dialogue pull-in; its Brick default is OFF. AUTO RESUME
defaults ON and restores a separate validated recovery session after graceful
close. An optional NextUI game-list launcher shares this pak's data root.
Power handling uses NextUI's real suspend helper, with pause/audio/input
restoration. Physical power, wake, audible audio and RTC still await Brick
testing. See LIFECYCLE.md for setup, source provenance and limitations.

Phase 15 adds original LCD/SCANLINES effects, GRID strengths, GBA COLORS and
unfiltered STRETCH in Menu > Settings. Choices persist per ROM. Start in the
companion Bag opens the real game's Bag on normal Polished Crystal 3.2.3.
This safe controller-only extension does not yet use or target companion items.
See FUNCTIONAL_AUDIT.md and ACTIONS.md for supported features and restrictions.

Phase 14 adapts Android's Guide page selector and inline rows, combining HERE's
encounters, people and items. It preserves unseen/hidden entry names until you
reveal them, uses the original owned balls and per-entry Pokémon icons, and
lists boss teams directly with hint/answer steps. Full Android parity,
performance optimization and power integration remain unfinished.

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
By default, original aspect and unfiltered integer scaling are preserved.
Optional effects and STRETCH are selected in Menu > Settings. FILTER requires
ORIGINAL aspect; EXPAND/ZOOM still work with these effects.
No Android runtime, touchscreen or mouse is used.

When updating an existing installation, merge the updated files into the pak
and keep your `roms`, `saves`, `states`, `recovery` and `display` folders and `settings.ini`.

For direct game-list launching, also install `Emus/tg5040/POKEDAISY.pak` and
put your game in a folder such as `Roms/Polished Crystal (POKEDAISY)/`.
Keep the Tools pak installed; the optional wrapper calls the same executable.

## Controls

| Button | Gameplay | Companion / menus |
| --- | --- | --- |
| D-pad | Game directions | Select a party member or move focus |
| A / B | Game A / B | Confirm / back |
| Start / Select | Game Start / Select | Start on Bag: original game Bag (verified normal 3.2.3); Select retains screen action |
| X | Open companion | Return to game |
| L3 | Quick-menu ring | Open / close quick menu |
| R3 | Cycle Original / Expand / Zoom | Display also available in game menu |
| Y | Toggle fast-forward (2x default) | Screen-specific alternate action |
| L1 / R1 | Game L / R on GBA | Previous / next section in either companion |
| Menu | Open game menu | Open / close game menu |
| Select + Start held together | Open game menu | Open / close game menu |
| L2 / R2, while paused | No action during gameplay | Confirm save / load state |
| Power (NextUI; hardware verification pending) | Short release: real suspend; 1s hold: graceful shutdown request | Same behavior |

The game menu also offers resume, party, state saving/loading, fast-forward,
Close game and About. Loading a state and closing require confirmation. AUTO
RESUME saves a recovery session on graceful close; continue saving in-game too.
Hold B at startup or launch with `--fresh` to bypass loading a suspect recovery.
AUTO RESUME OFF disables automatic loading and writing without deleting files.
Sudden power loss, a crash or forced termination cannot trigger a final save.
In Polished Crystal, L/R cycles Party, Bag, Battle, Dex, Map and Guide. In Party, the D-pad
follows the two-column grid and A opens a summary. Within a summary, Up/Down changes Pokemon, Left/Right selects a move, A opens its ROM description, Y cycles Info/Weaknesses/Resistances/Immunities, and Select pages long matchup lists. B returns one level. Eggs conceal their moves. In Bag, Left/Right changes
pocket and Up/Down scrolls through items. The Bag shows live quantities and
money, including medicine grouped under Items, plus Balls, TMs/HMs, Berries and
Key items. Press A for an item description read from the matching ROM. The Bag remains read-only; upstream does not provide Polished item-use actions.
Battle uses Y to cycle INFO, SUGGESTIONS, FOE DETAILS, FOE TEAM and FOE TYPES. FOE TYPES uses Left/Right for weaknesses, resistances and immunities, with Up/Down for more rows. INFO rates damaging moves against the selected foe. Suggestions uses upstream’s power/type/defensive-risk heuristic, not a full damage calculation. Foe Details shows actual active-battler stats, held item and moves; reserve stats are omitted because the Polished party reader does not provide them. On Foe Team use D-pad and A to inspect a member. Unannounced slots remain unnamed until selected; Select follows the live foe again. B returns to INFO, then gameplay. No battle commands or memory writes are introduced.
In the Dex list, Y cycles ALL, CAUGHT, SEEN ONLY (seen but not caught) and NOT SEEN. Filters are retained when returning from an entry or switching companion sections. Empty results cannot open an entry. Dex shows live seen/caught flags in Polished Crystal's 289-entry order. Up/Down chooses, Left/Right pages and A opens an entry. Up/Down browses entries and B returns to the list. Entry sprites, category, text, size and base stats read from the matching ROM. Y switches to the full-text/abilities page.
Map displays Johto, Kanto and Orange with original ROM palettes, tile flips and player art. The D-pad moves its cursor; A changes region, Y recenters on the player, and Select opens Places. A on a place shows it on the map. Map is read-only; Fly is not implemented.
The Guide uses Android's HERE, NEXT BOSS, TIPS and WHERE IS pages with inline
entries. HERE combines present encounter methods, PEOPLE and ITEMS. Unseen
encounters and uncollected hidden-item names stay concealed until revealed.
NEXT BOSS lists each remaining team's Pokémon directly, with a type hint and
species/moves/held-item answer. See Phase 14 Guide controls below. The Guide
follows the game clock, badges and read-only progress; upstream marks its data
unverified, so entries may contain mistakes.
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
- `recovery/<ROM SHA-1>.session` for automatic recovery, separate from both above.
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
Physical L3/R3 detection, expansion performance and suspend behavior still need a hardware retest. The supplied supported 3.2.3 ROM passes all 289 dex texts/front pictures, 373 item/TM/key descriptions, all three maps, and a normal-input new game to the starter. The ARM frontend reads that actual party and passes pause/resume and state tests without fixture injection.
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

R3 cycles Original (160×144, 5× on Brick), Expand (204×152, 5×) and Zoom (256×192, 4×). Expansion is initially enabled only for the verified normal Polished Crystal 3.2.3 ROM, SHA1 `6930b48af5844d373e3c9130f26d6dd1084cf4ed`. Other companion-supported versions retain normal gameplay. Menus, dialogue, battles and uncertain map matches automatically use the original viewport. Display mode is remembered per ROM content hash, including after a rename. Missing or corrupt preferences use Original. Preferences are stored separately under `display/`; state loading preserves the selected mode but resets the renderer cache. This is an initial expansion implementation: loaded NPCs are drawn, but unspawned and connected-map NPCs are not reconstructed. Indoor laboratory rendering is verified; broad outdoor/connection and hardware performance testing remain. Menu > Display offers the same cycle when R3 is unavailable.

L3/R3 are discovered from SDL controller bindings or Linux joystick/evdev button codes. The launch log records the result. If a driver does not identify them, set `POCKET_L3_BUTTON` / `POCKET_R3_BUTTON` to its raw SDL button numbers (0–255) in launch.sh, or use Menu > Quick menu. No unverified raw index is assumed for these buttons. Keyboard F3 opens the quick menu; F4 cycles the display for development.

## Existing companion features

Adds the four upstream Pokedex filters, live-foe type-matchup groups and Guide best-party/boss-ace level comparison. All Phase 8 expansion and quick-menu features remain included. No GBA feature development or game-memory writes are added. Full Android parity remains unfinished; see PARITY.md.

## Phase 11 integration

Upstream history is merged through `54d8f4867501feb32b7ee139a5616b7c6301f044` (1.3.1-era source). The native build reuses upstream `pk_savevf.c` unchanged for memory-mapping fallback. `host_io.c` adds shared error handling around the underlying save file, and content-keyed atomic display preferences. A save must still open read/write. I/O failures stop emulation and are reported; the existing pre-launch `.bak` is retained when one was made. This is not power-loss protection. No battery-save migration is performed.

Android-only 1.3.1 features remain available in the source for later adaptation; merging their source does not enable MAKEOVER, DS or new GBA companions on Brick. Polished's generated decoding/data remains isolated from the Linux host.

## Phase 11 party and summary

Party slots use the upstream PartyScreen geometry/palette and pixel HP label. The selected icon bobs at the upstream ten-frame cadence while the game remains paused. No extra Poke Ball art is introduced: the Polished palette makes it transparent. Summary uses the game's real total EXP, the upstream six experience tables and Polished's own growth rates. NEXT shows the remaining EXP; level 100 shows MAX. Eggs hide the EXP bar. Suggestions' OUT marker uses the game-specific generated current-party-index address and is omitted if the index cannot be read safely.

The upstream Gen 2 party slots do not have an EXP bar, and its Dex list does not provide name search; earlier parity notes suggesting otherwise were corrected.

## Phase 12 battle and map

Battle INFO, SUGGESTIONS, FOE DETAILS and revealed FOE TEAM rows show health with the same HP lettering and Polished colors as the party. Names, level/OUT and health have separate space in team and suggestion rows. Unknown foes remain concealed. Map uses the upstream four-corner cursor, pulsing every 333 ms around the selected landmark or empty cell; its center stays transparent. The UI clock animates it while emulation stays paused.

The parity checklist now reflects two upstream Gen 2 boundaries: there are no item icons and no announced-next-foe index in its reader. Gen 2 Party uses SpeciesIcon's first animation frame with selected-icon bobbing, which is already adapted. These are not missing features to invent. Additional battle panel styling and broader lifecycle/hardware work remain open.

## Phase 13 visual pass: Bag and Dex list

Bag opens All and shows only meaningful occupied-pocket tabs, like Android. Left/Right changes pocket; Up/Down moves the cursor and follows the selected item's description. A toggles the inline description, B closes it before returning to play, and Select pages a long description without losing text. Y cycles Default / Name / Type / Count sorting; sort keeps the same selected stack. Sorting is a read-only companion view, never a game-bag reorder. Individual stacks remain distinct even if they share an item id. Favorites are not adapted yet.

Dex now places SEEN/OWNED counts and all four filters beside the numbered icon list. Caught/seen marks reuse upstream pixel artwork; unencountered entries remain visible but dim, as Android's DexRow does. The Brick stacks each row's number and name to keep full-size ROM text readable. Up/Down scrolls one entry, Left/Right moves five entries, Y changes filter and A opens the existing entry page. Entry-page composition still needs a later visual pass.

Top navigation retains the existing five-chip shoulder workflow on Brick, now with upstream Game Boy chip frames and colors. Android's bottom placement, gear/MORE arrangement and complete settings/state browser are not claimed. Guide and battle panel composition also remain open. We will finish those visual comparisons before the performance and power/suspend/exit milestone; GBA companion expansion follows that work.

## Phase 14 Guide controls

L/R reaches GUIDE. Left focuses its page selector; Up/Down changes pages there,
and Right or A returns to the entries. Y is a direct next-page shortcut.
Up/Down walks entries, skipping headings. A cycles closed, hint, answer, closed
(or directly to the answer if the source has no hint). Reveals stay inline and
only one entry opens at once. Select scrolls a tall answer; B closes the current
reveal, then returns to gameplay. X always returns to gameplay.

HERE includes all encounter methods present on the current map, then PEOPLE
and ITEMS for the area including its buildings. Collected area rows sort last.
The guide uses upstream's data, which upstream labels AI-assisted and unverified.
Preserve `roms`, `saves`, `states`, `display` and `settings.ini` when updating.
