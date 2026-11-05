# DS companion/settings update

This follows the first DS candidate at b64a76e. The owner verified native NXRedux
launch, fast-forward, companion entry and screen swapping on the actual Brick.
That hardware result applies to the previous candidate; this UI update needs
another hardware test.

Compared against thwonp/pokedaisy-plorp feat/libretro-core at 4ab40fa:
- The DS companion now uses the same portable frame, bottom tab chips and gear
  as the GB/GBA companion. Only implemented DS tabs appear: PARTY, BAG, MAP, DEX,
  SETTINGS. Party navigation follows its visible two-column layout.
- The existing DS readers, ROM icons, bag, Dex and map adapters are retained.
  There is no second companion data implementation and no generated-data update.
- Settings are available even before the game has loaded a valid party.
- Multi-choice settings use the shared choice-window renderer. A opens, D-pad
  highlights, A applies, B cancels. ON/OFF choices toggle directly. D-pad repeats
  while held. Preferences are saved after each confirmed change.

## Settings inside the companion

| Row | Effect |
| --- | --- |
| DS LAYOUT | AUTO, TOP, BOTTOM, SIDE BY SIDE, STACKED |
| AUTO BATTLES | In AUTO layout, switch to the touch screen for command/move panels |
| PARTY HUD | Show/hide the existing shared party HUD over top-only gameplay |
| COMPANION BUTTON | L2 (default), R2, X or Y |
| SCREEN BUTTON | R2 (default), L2, X, Y or NONE |
| STYLUS SPEED | SLOW (1), NORMAL (3), FAST (6 DS pixels per frame) |
| RESET SETTINGS | CANCEL first; reset this core's preferences after confirmation |

L1/R1 cycle through the four tabs and gear. From PARTY, L1 goes directly to
SETTINGS. B closes a choice without applying it, backs out of a summary/details,
then returns to the game. From SETTINGS, B returns to PARTY. The selected
companion button always toggles the overlay. Assigning X/Y reserves that native
DS button for the chosen overlay shortcut; the default L2/R2 preserves all four
face buttons. A screen shortcut cannot take the companion's button. Moving the
companion shortcut onto the current screen shortcut swaps their old assignments.

Preferences use the shared pd_settings loader/saver plus DS-specific keys in
`<frontend save directory>/pokedaisy/settings.ini` (normally
`Saves/PDDS/pokedaisy/settings.ini`). Writes use a temporary file, flush/fsync
and atomic rename. Errors are displayed instead of pretending a save succeeded.
Unknown/bad values are bounded. The old layout/HUD core options seed defaults
only when no saved companion settings exist. Re-selecting a layout in SETTINGS
clears a temporary manual-screen override, even if the selected value is AUTO
again. Loading/resetting emulator state closes stale dialogs, not preferences.

NXRedux retains state slots, quick resume, fast-forward, shaders, scaling and
exit. Their playback controls are not duplicated as inert companion settings.
This is a companion/settings alignment pass, not full Android/Thwonp parity:
custom DS quick-menu actions, HUD layouts/looks/cursor, trainer card, guide and
advanced party summaries remain future work. DS expansion is unchanged/absent.

## Validation

Native CTest: diagnostic generation, libretro lifecycle/state/geometry contract,
legacy shared UI behavior, captured Platinum companion data, and the new settings
controller tests. Settings tests exercise choices/cancel, correct grid movement,
button conflicts, atomic persistence/reload, cancel-first reset, invalid values
and storage-failure feedback.

ARM build uses the same pinned DSperate/libretro wrapper and TrimUI SDK as the
first candidate. AArch64/QEMU checks and real-Platinum captures are recorded in
the accompanying validation files. Emulator serialization and battery-save
formats are unchanged; no separate save writer was added. New actual-Brick
checks: open the gear, change/reopen a preference, test a picker cancel, confirm
L2/R2 defaults still work, then test a native state/quick resume.


The unified integration branch adds Select + companion-button quick menu,
Ring/Grid selection, and an independent pokedaisy-ds settings namespace. See
../libretro-unified/README.md for migration and current feature boundaries.
