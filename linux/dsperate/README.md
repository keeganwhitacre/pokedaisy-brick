# Optional Linux DS backend

DSperate adds Nintendo DS emulation to the SDL Linux host. It is opt-in:
normal builds still use only mGBA for GB/GBC/GBA. The initial read-only
companion is limited to the exact Platinum USA rev1 ROM already identified
by the Android app. Other DS ROMs have emulation only, without a verified
companion or automatic battle-screen switching.

This contribution is a foundation for upstream development. It includes
native-screen emulation, controls, saves/states, optional clean-exit resume,
manual and automatic screen switching, a limited companion, the existing Linux
quick menu and party HUD, and native/TrimUI CI. It deliberately omits expanded
rendering, custom battle commands, and the experimental overlapping battle panel.

## Build

Follow `linux/README.md` for SDL2, compiler and TrimUI SDK dependencies. DS also
needs C++17. Initialize the pinned cores, then configure the optional backend:

```sh
git submodule update --init third_party/mgba third_party/dsperate
cmake -S linux -B linux/build-ds -DCMAKE_BUILD_TYPE=Release -DBUILD_LTO=OFF -DPOCKET_DSPERATE=ON
cmake --build linux/build-ds -j2
ctest --test-dir linux/build-ds --output-on-failure
python3 linux/package.py --dsperate linux/build-ds/pokedaisy-linux /tmp/ds-artifacts
```

For TrimUI, use the same SDK and `trimui-toolchain.cmake` documented for the
normal Linux host, adding `-DPOCKET_DSPERATE=ON`. The DS workflow builds native
and ARM versions, runs ARM tests under QEMU and packages both artifacts.
DSperate's pinned source is built without modifications. Its JIT/NEON paths
are used on ARM; QEMU timings do not establish physical handheld performance.
The SDK lacks GCC's LTO plugin, so the DS core's IPO is disabled; GCC before
9 additionally links `stdc++fs`.

Place your `.nds` ROMs in `DATA/roms`, or run:

```sh
linux/build-ds/pokedaisy-linux --data /path/to/test-data /path/to/Platinum.nds
```

The core supplies FreeBIOS and generated firmware. No commercial BIOS, ROM,
save or extracted artwork is included in the contribution or packages.

## Controls

| Control | Action |
| --- | --- |
| D-pad, A/B/X/Y, L/R, Start/Select | Native DS buttons |
| L2 | Shared quick menu; inside the companion, return to gameplay |
| R2 | Swap DS screens; in AUTO battles, temporarily override until the phase changes |
| Select + D-pad, Select + A | Move stylus cursor, touch while bottom screen is visible |
| Menu | Pause; B resumes; A requests save, X requests load, Start requests exit |

Quick menu: D-pad selects, A confirms, B/L2 closes. Entries are COMPANION,
SAVE STATE, LOAD STATE and LEAVE GAME. A missing state disables LOAD STATE.
The companion uses L/R tabs; A opens a party summary, B backs out. Left/right
switch summary info/moves, or page bag/dex lists. Map: D-pad moves over the
full grid; A recenters on the player. The map is read-only; no Fly action.

SET scrolls through SCREEN, BATTLES, QUICK RESUME, SAVE ON QUIT, HUD,
QUICK MENU (RING/GRID) and FILTER (OFF/LCD). Common preferences use the existing
`pd_settings` parser and `DATA/settings.ini`, shared with Polished/GBA. The DS
host writes them atomically; it uses the existing quick-menu/HUD renderers and
LCD mask. Its companion/SET screen is still the basic DS frontend, not the full
shared GBA companion. Only SCREEN and BATTLES remain in `ds-settings.ini`;
legacy resume is imported only when no common settings file exists.
AUTO is the new-install default, limited to the identified Platinum ROM. It shows the
native bottom screen while the battle waits for selection, and the top during
animations. It does not replace game menus or independently perform actions.
Old experimental `expand` preferences are ignored; old `battle=2` becomes AUTO.

Keyboard: Z/X = A/B, S/A = X/Y, Q/E = L/R, Enter/Backspace = Start/Select,
Tab = L2, Space = R2, Escape = Menu. All four native face buttons remain usable.

## Saves and recovery

- Battery save: `DATA/saves/ROM.nds.sav` (the ROM basename includes `.nds`).
- Manual state: `DATA/states/ROM.nds.dsperate.ss`.
- Optional resume: `DATA/states/ROM.nds.dsperate.resume.ss`.

Use a separate data folder and copies of personal saves. Existing emulator
folders are not automatically imported. Startup uses the host's bounded save
backup policy. Battery writes settle before atomic replacement; failure leaves
the previous disk copy. Continue using the game's own Save menu.

QUICK RESUME and SAVE ON QUIT default OFF and are independent: the former
loads the automatic slot and selects the last game at startup; the latter
captures that slot on clean exit. Enable both for the complete resume flow. A forced
power-off cannot capture it. A newer battery-save timestamp takes precedence.
Invalid resume states are loaded into a separate console and fall back to a
fresh boot. Manual loads likewise validate in a separate console, retaining the
current game on failure. DSperate validates ROM identity and BIOS before loading.
Manual save/load and exit require a cancel-first confirmation (Left/Right
chooses YES; A confirms; B cancels). Held overlay buttons are blocked until
release. `--no-resume` bypasses automatic loading, last-game selection and capture. `POCKET_DS_RESUME=0` bypasses resume and capture for recovery. Resume
states contain battery RAM; do not treat them as portable archival saves or
assume compatibility with another emulator/version.

## Shared data and interfaces for review

The adapter reuses `pk_nds_field.cpp`'s existing game/state logic through RAM
callbacks. CMake currently extracts its RAM-only prefix and excludes the
melonDS GPU helper. A missing extraction marker fails configuration. This is
an interim integration point for upstream to replace with a portable target.
The two missing Platinum battle-context/command offsets are a small shared
prerequisite; no `3ds/core` or `pk_wide_gb.c` modifications are needed.

`gen_data.py` derives the checked-in `data_gen.h` from existing Kotlin Gen 4
configuration, names, character maps, map metadata and palette declarations.
`--check` verifies it in CI; regenerate after changing those upstream sources.
There is no separately maintained registry of game addresses. Gen 4's party
and bag decoding is adapted into `companion.cpp`; this is not yet a universal
shared reader. The intended long-term snapshot/data interface is for upstream
review, not a competing gamedata architecture.

Town Map and party icons are decoded from the user's ROM using NdsArt.kt's
container/tile rules. The HUD and quick menu use the existing Linux renderers.
A small HUD art callback supplies DS icons without treating national DS IDs
as GBA internal IDs (Burmy's 412 would otherwise collide with the GBA egg).
Party HP/level/status, summary information/moves, bag, dex flags/counts,
trainer/location, and map/player/arrival markers are implemented. Full dex
entries, field actions, battle telemetry and wider ROM compatibility remain
incomplete. DS has a separate event loop and basic companion screens; these
are explicit integration points for future upstream work.

## Validation

Tests use an original diagnostic NDS cart, upstream captured-RAM fixtures,
real process restarts, corrupt resume inputs and actual SDL output. Display
regression checks inherited launcher scaling at 1024x768, 1280x720 and 640x480.
Host input tests cover quick-menu navigation, state save/load and leaving.
Existing Linux GB/GBC/GBA tests run alongside the DS tests.

Optional personal-ROM checks: `linux-ds-test FIXTURE DIAGNOSTIC_ROM ROM.nds`
also validates real party icons and Town Map assets. No personal ROM is needed
for CI. `POCKET_DS_KEYS`, `POCKET_DS_HOST_KEYS`, `POCKET_DS_COMPANION_AT`,
`POCKET_DS_QUICK_AT` and `--frames/--shot` are headless integration controls.
`POCKET_DS_BATTLE=0|1` overrides the battle display preference for a test run.

The preceding experiment was tested by its contributor on a physical Brick
with expansion off. This newly separated PR branch needs fresh device checks,
including sustained speed, audio, menu return, saving, sleep/wake and longer gameplay. Neither
QEMU nor native captures establish hardware performance or support for other
handhelds. DSperate, FreeBIOS and bundled dependency notices ship in DS packages.

## Polished foundation and NextUI install

This DS development line starts exactly at `ec441a754f8de4cd231dbc126ef9ac65e2bd5992`.
The original is tagged `brick/polished-foundation-ec441a7`; `brick/dev` is unchanged.
The mGBA/Polished loop, companion, expansion, field actions, Potion targeting,
Ring/Grid, optional HUD/LCD and resume behavior remain in the same executable.
DS expansion and custom single-screen battle controls are absent.

For an ARM build, `python3 linux/package.py --nextui --dsperate BINARY OUTPUT`
creates `PokeDaisy-DS.pak.zip`. Extract `PokeDaisy-DS.pak` into
`Tools/tg5040/` on a NextUI Brick SD card. Keep the Polished pak separately.
Launch once, then place private `.nds` files in the new pak's `roms/` folder.
A backed-up raw DS save goes in `saves/`, named exactly `ROM.nds.sav`.
No automatic save migration occurs. No commercial BIOS is required.
The package includes dependency license texts and exact source revisions.

Minimize/focus loss pauses DS; Menu/B resumes. Controller hotplug is passed to
the Linux host, and disconnect clears keys and pauses. This is not verified
NextUI power-button/sleep integration. The host paces unexpanded gameplay at
59.8261 Hz with accumulated deadlines; finite `--frames` tests are unthrottled.
Snapshot decoding is limited to once per 12 host iterations during gameplay,
with an immediate refresh on companion entry, reducing repeated decoding work.
These choices are not a measurement of physical Brick speed or battery life.

Current validation and audit details are in `BASELINE-VALIDATION.md`.

## Platinum companion browsing parity

PARTY now uses a two-column, six-card layout with ROM-derived party icons,
status labels and HP bars. Up/Down moves between rows; Left/Right moves one
slot. A opens the existing summary and B returns to the cards. Summary controls
are unchanged. Icons use the existing DS ROM decoder, not GBA species tables.

BAG: Left/Right cycles ALL and the eight save pockets in the app's Gen4Reader
order (items, key items, TMs/HMs, mail, medicine, berries, balls, battle items).
Up/Down scrolls within the selected pocket; changing pocket resets the scroll.
Empty pockets have an explicit message. This is browsing only, not item use.

DEX: Select cycles ALL / SEEN / CAUGHT. Up/Down scrolls and Left/Right pages
within the filtered list. The original national number is retained. Changing
filter resets the scroll; empty and boundary selections remain bounded.

Regression checks partition the upstream Platinum RAM fixture's bag into all
pockets, verify its seen/caught sets and reject out-of-range dex indices. Private
captures additionally exercise the attached retail ROM/save's party, card focus,
summary, Medicine pocket and Caught filter. No new address tables or gameplay
writes are introduced. These additions do not establish physical Brick results.
