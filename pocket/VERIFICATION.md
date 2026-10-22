# Phase 9 verification

Phase 7 adds the L3 quick menu: original Android state/FF/companion pixel icons,
D-pad ring navigation, genuine paused gameplay at its existing physical scale,
disabled missing-state load, save/load confirmation and fast-forward/companion
actions. Existing gameplay/companion textures retain their opaque RGB888 path;
the additional overlay uses explicit ARGB alpha. Phase 8 adds R3 Original/Expand/Zoom (see results below).

Native Linux and the actual NextUI ARM build pass the new quick-menu test. The
same real supported 3.2.3 ROM and genuine fresh-game starter state from the
previous pass were loaded, without fixture/RAM injection. Tests check paused
core frame counters, raw L3 and keyboard input, all directions/actions,
confirmation/cancel and state round-trip, Menu fallback/back behavior, resuming
and repeated open/close. Synthetic Linux key-bitmaps test SDL 2.26 evdev button
ordering; real Brick device discovery remains unverified. The raw-button
override is tested with buttons 12/13, which are test values, not assumed Brick
indices. Native diagnostic GB/GBA regressions pass. ASan quick-menu testing
completed without ASan errors; fully instrumented pinned mGBA still emits its
known UBSan signed-arithmetic reports. No whole-core UBSan-clean or full leak
check is claimed. LeakSanitizer was disabled.

`runtime-phase7-quick*.png` captures show the actual supplied-ROM game paused
behind the menu. They are software SDL captures, not photographs or hardware
proof. Physical L3 detection, overlay alpha on the device backend, audible
resume and sustained performance need a Brick retest. Use Menu > Quick menu
if a driver does not label its L3 button; launch.log records detected bindings.

Quick-menu icons are regenerated with `python3 pocket/gen_quick.py --check`.
No PC/Fly/Bike/Rod/Repel/game-setting action is simulated. Existing state/SRAM
rules remain unchanged, and all tests use separate disposable save directories.

The Phase 6 results below describe the retained battle milestone.

## Phase 6

Phase 6 adds Battle INFO matchup labels, Suggestions, Foe Details and Foe Team. Native and ARM builds/decoder tests pass, including faithful moves, zero PP, fainted/egg exclusion, stable ranking ties, type immunity/dual types, seen-foe tracking/reset and controller back/selection. Active stats and held items are checked against upstream captured raw structs. ASan/UBSan decoder/render checks include matching-ROM rendering. The supplied 3.2.3 fresh-game and ARM SDL pause/resume path also pass.

Battle screenshots named `runtime-phase6-*-fixture.png` combine upstream captured battle RAM with the supplied matching ROM’s graphics/font. They are regression captures, not newly played battles in the user’s save. The private fresh game remains starter-stage; no newly played battle or physical Phase 6 Brick test is claimed.

The Phase 5 results below remain applicable to the reused Guide, ROM codecs and emulator foundation.

## Result and limits

Phase 5 adds Polished Crystal's read-only Guide: HERE encounters and area finds,
NEXT BOSS, all seven TIPS and generated WHERE IS lists. The definitions and
selection rules are adapted from current upstream Kotlin sources, with schema
assertions and recorded input hashes. Guide art/text uses the same original ROM
font and companion style as Phase 4. There are no new memory writes.

The supplied 3.2.3 ROM matches upstream SHA-1
`6930b48af5844d373e3c9130f26d6dd1084cf4ed`; its bytes remain unchanged.
A fresh game was played with normal emulator key presses, obtaining Cyndaquil
Lv5 HP19/19 EXP135 at Elm's lab, map 24.5. Its actual received-Cyndaquil event is
marked complete in HERE, badges are zero, and NEXT BOSS is Falkner. This run
uses no injected party, bag, progress flags or external save. Separate test states
and diagnostics never touch the user's existing saves.

The user confirmed base gameplay/party/sprites on a physical Brick. The agent
has no Brick hardware. Phase 6 requires a device retest; Linux/QEMU audio uses
the dummy backend, so audible quality, real controls/power and performance are
not newly hardware verified. Full Android parity is not claimed; see PARITY.md.

## Tests

- Native Linux and actual Cortex-A53/AArch64 NextUI GCC 8.3 + TrimUI SDK builds.
- Native and ARM/QEMU decoder tests: 2,776 map/method/clock cases and 36
  time-dependent fishing slots; merged encounter odds/levels, evening/night rule,
  badge-adjusted levels, trade/badge flag banks, League/rematch completion,
  Where Is ordering/deduplication and queued-input bounds.
- Five normal/faithful captured party/bag builds and wild/fainted/trainer battle
  captures; bag capacity/terminator, dex mapping, plain/Huffman text, corrupt
  stream and pointer boundaries, egg privacy and SHA-1 vectors.
- Supplied-ROM native and ARM/QEMU normal-input new-game test, actual guide
  progress and page/reveal/back/scroll/section wrap controls; all 289 dex
  texts/front pictures, 373 item/TM/key descriptions and three maps decode.
- SDL frontend loaded the genuine test state, read its actual party, produced
  nonblack gameplay, opened/closed via raw joystick X/B, paused/resumed core
  frame counters and passed a state round-trip. QEMU fullscreen uses 1024x768,
  GBC 800x720 integer gameplay. The production executable has no test injection.
- Diagnostic emulator tests cover 100 companion toggles, battery marker,
  state/SRAM behavior and original GBA companion/emulation (960x640 on target).
- ASan/UBSan decoder tests pass. A broader fully instrumented mGBA real-ROM run
  completed without ASan errors but reported existing signed shifts/overflows
  in pinned mGBA's GB audio/video implementation; see sanitizer log. Therefore
  the whole emulation core is not described as UBSan-clean. LeakSanitizer was
  disabled; no full leak-profile claim.
- Generator --check, diff whitespace and launcher shell checks pass. The SDK
  setup helper was exercised against the pinned archives before target rebuild.

## Captures

`runtime-live323-guide-*.png` are actual 512x384 renders using the fresh-game
memory and the user's 3.2.3 ROM. The wild page accurately reports no grass/cave
encounters indoors. The team answer scrolls to show the entire boss team.
Other runtime-live323 captures show the same real fresh-game party/map/dex.
Captures named fixture use upstream captured memory and are regression evidence,
not a claim that the user's save contains those states. No ROM, save or private
state is distributed.

## Reproduce

```
git submodule update --init third_party/mgba
python3 pocket/gen_polished.py --check
cmake -S pocket -B pocket/build-native -DCMAKE_BUILD_TYPE=Release -DPOCKET_TESTS=ON -DBUILD_LTO=OFF
cmake --build pocket/build-native -j4
cd pocket
python3 make_diagnostic.py
build-native/pocket-decoder-test ../app/src/test/resources/fixtures
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build-native/pocket-runtime-test
# Optional private, supported normal 3.2.3 ROM; never published:
build-native/pocket-rom-test /private/polishedcrystal-3.2.3.gbc boot-polished-newgame.txt /private/test.ss
```

For ARM use nextui-toolchain.cmake and prefix tests with qemu-aarch64-static -L
<target sysroot>; SDL_VIDEODRIVER=offscreen, SDL_RENDER_DRIVER=software,
SDL_AUDIODRIVER=dummy and POCKET_FULLSCREEN=1. These are test-only backend choices.
The install launcher continues to use the actual NextUI backends.

## Next priorities

1. Hardware Guide/control/normal-save/reload/exit retest.
2. Remaining battle layout, weaknesses, active-party marker and next-foe selection.
3. Dex filters, Guide per-entry reveals/icons and remaining summary/EXP styling.
4. Multi-slot state browser, long-session performance, RTC and suspend/power.

The source is prepared for the user's pokedaisy-brick fork, retaining the shared
upstream ancestor. Remote CI results must be read separately from local tests.

## Phase 8 results (2026-10-22)

Linux/x86-64 and actual NextUI GCC 8.3 Cortex-A53/AArch64 builds compile. The private user-supplied normal 3.2.3 ROM passes the original normal-input new-game route: Cyndaquil Lv5, 19/19 HP; 289 dex texts/front images, 373 descriptions, three region maps. No commercial ROM, extracted art, battery save or state is distributed.

The SDL runtime test dismisses actual starter-event dialogue with ordinary input, then checks R3 via keyboard and raw button override. Expand draws 1020×760 and Zoom 1024×768 on the 1024×768 test surface; Original remains 800×720. Both expanded modes reach a 1000/1000 map-tile match. Every displayed expanded frame asserts that the entire original 160×144 PPU viewport is unchanged. Companion and quick-menu frames pause the core and resume correctly. Loading a state invalidates the render cache. Real runtime captures `runtime-phase8-expand.png` and `runtime-phase8-zoom.png` show Elm's laboratory, not synthetic game data.

The ordinary captured-memory decoder, GB/GBA diagnostic runtime and L3 quick-menu regressions pass. Expanded modes were also run with the compiled ARM executable under QEMU; this is not physical hardware or performance validation. SDL software/offscreen rendering and dummy audio are test backends only. In-game menu fallback and post-state reset are checked by the extended private-ROM runtime test.

Reproduce private expansion check after the normal new-game test creates a separate test state:

```sh
SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy POCKET_FULLSCREEN=1 POCKET_TEST_WIDE=1 POCKET_TEST_ROM=/private/game.gbc POCKET_TEST_STATE=/private/test.ss pocket/build-native/pocket-runtime-test
```

Limitations: only normal 3.2.3 expansion is verified; laboratory coverage does not establish all-map correctness. Connected-map terrain is adapted but lacks a broad playthrough. Unspawned and connected-map NPCs are deliberately omitted. Physical R3 discovery, long-run speed/audio, RTC, suspend and power-loss safety still need Brick testing. No new hardware test or full Android parity is claimed. See PARITY.md for the remaining Polished work before GBA.

## Phase 9 results (2026-10-22)

Linux and actual NextUI ARM builds compile. Decoder tests cover all four exact upstream Dex predicates, filtered cursor boundaries, return-to-list retention, empty-list entry blocking, battle 4x/2x/0x classification and sorting, five-page cycling and Guide egg exclusion/unavailable-party behavior. The supplied normal 3.2.3 ROM passes the normal-input new-game test and produces actual-memory Dex-filter and boss-level captures at 512x384. Battle-type captures use upstream captured trainer memory with the supplied ROM's font/art; they are explicitly marked `fixture` and are not presented as a live trainer playthrough.

The ARM executable also passes these decoder tests and GB/GBA runtime/quick-menu regressions under QEMU. No physical Brick testing occurred in this pass. New captures were visually inspected at 512x384. No ROM, extracted art or test saves/states are packaged. Native controls, state/save behavior and the Phase 8 display architecture remain intact. Full Android parity and broad expansion-map coverage remain open in PARITY.md.
