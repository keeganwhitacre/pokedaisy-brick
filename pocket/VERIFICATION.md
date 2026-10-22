# Phase 5 verification

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
has no Brick hardware. Phase 5 requires a device retest; Linux/QEMU audio uses
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
2. Remaining Polished Battle INFO, foe-team/details and suggestion panes.
3. Dex filters, Guide per-entry reveals/icons and remaining summary/EXP styling.
4. Multi-slot state browser, long-session performance, RTC and suspend/power.

The source is prepared for the user's pokedaisy-brick fork, retaining the shared
upstream ancestor. Remote CI results must be read separately from local tests.
