# Phase 16 verification

Native Linux and actual NextUI Cortex-A53/AArch64 GCC 8.3 builds pass. ARM
executables run against the actual TrimUI/NextUI SDK sysroot under
qemu-aarch64-static with software/offscreen SDL and dummy audio. Neither this
nor the simulated suspend callback is physical Brick validation.

The private normal Polished Crystal 3.2.3 ROM matches upstream SHA-1
6930b48af5844d373e3c9130f26d6dd1084cf4ed. The normal-input new-game harness
creates a genuine starter; tests never open the player's battery files. The
session harness observes 1,972 expanded TEXT frames and 428 expanded FIELD
frames through 2,400 normal-input frames of the starter conversation. Every
original PPU center pixel is asserted unchanged on every expanded frame.
AUTO ZOOM IN ON pulls in; state load resets then restores safe expansion.
The dialogue, auto-zoom, state and field PNGs were inspected. These are actual
native software SDL captures from that game, not new Android GL comparisons.

Native and ARM tests pass recovery roundtrip with exact frame counters;
failed SRAM-sync preservation; corrupt/truncated/wrong-ROM rejection before
core mutation; changed-battery rejection; manual-state separation; and actual
frontend graceful exit/relaunch, AUTO RESUME OFF and --fresh bypass. The test
also inspects the actual envelope payload to assert it contains no savedata.
See LIFECYCLE.md for content versus RTC-trailer checks and atomic-write limits.

Synthetic power events verify immediate pause between frames, zero input,
action cancellation, closed audio during the substituted suspend callback,
no sleeping frame advancement, queued-input removal, reopened audio, failure
recovery, wake debounce and one-second long-hold detection. No test invokes
the real suspend helper or writes the shutdown marker. Physical short/long
power, display restoration, audible quality, real control timing and elapsed
RTC remain awaiting Brick testing. NextUI helper/launcher source inspection
does not certify firmware behavior.

Existing host save I/O faults/fallback, decoder/captured Guide, GB/GBA execution,
100 companion toggles, quick-ring confirmation/cancel/state roundtrip,
expanded Original/Expand/Zoom cycling, paused companion/ring, PPU preservation,
state-load cache reset and display persistence across reopen pass on native
and ARM/QEMU. Controller tests cover the original filter settings plus both
new toggles, per-ROM persistence and companion START -> the actual game Bag.
The options harness now advances a fresh starter out of dialogue using normal
buttons before testing the context-restricted Bag opener. The production
context guard is unchanged. Phase 15 preference migration preserves filters.

The loaded-object layout and sprite flags were checked against Polished's
actual release source. No connected-map NPC fix or real-ROM connection-travel
validation is claimed: that remains a gap requiring suitable exploration/test
states. No objects, terrain, game actions or memory addresses were invented.
The new lifecycle code has not received a fresh sanitizer certification.
Earlier sanitizer results below apply only to their documented earlier runs.

Reproduce with POCKET_TESTS=ON and the commands in LIFECYCLE.md. The package
includes phase16 test logs and screenshots, with no ROMs, player saves, states
or recovery files. Source checkpoints are on phase16-nextui-checkpoints;
main retains the previously tested Phase 15 baseline.

## Phase 15 verification (retained)

Phase 15 is an incremental functional milestone, not full Android parity.
Native Linux and actual NextUI AArch64 builds pass the controller-only Bag
opener, display reference/persistence checks and host/menu integration. The
real normal 3.2.3 ROM advances Elm dialogue using ordinary buttons; the new
adapter opens its real empty Bag in 61 emulated frames without game-memory
writes. Unknown ROM, dialogue and already-open-menu requests reject; cancellation
and a no-progress timeout return zero synthetic keys. No healing-item use,
party target, held-item action or battle-command execution is claimed.

Controller tests select Game > Settings, change FILTER/GRID/GBA COLORS, verify
per-ROM persistence and refuse unsupported STRETCH+FILTER. Start from the
companion Bag then opens the running game's Bag. LCD/SCANLINES masks use the
source texel equations; color reference vectors test the GLSL matrix ordering.
Malformed/truncated preference files fall back safely. The existing *.mode,
INI, SRAM/backup and state locations remain unchanged.

Native and ARM/QEMU host I/O, captured decoder/Guide, real GB/GBA execution,
100 companion toggles, real state round-trips, quick ring/confirmations and
expanded/zoomed original-centre/state-reset/reopen regressions pass. The
new functional harness also passes with ASan/UBSan instrumentation on the host,
dispatcher and display code; the linked mGBA and older companion objects are
not instrumented in this run. Leak detection is disabled. This is not a
whole-core sanitizer certification.

`runtime-phase15-*.png` show actual emulator frames, the settings menu and
companion-to-game Bag result. Captures were inspected. Native/ARM software SDL
outputs agree exactly for unfiltered, GBA COLORS and STRETCH; LCD/SCANLINES
blend outputs differ by at most one channel level between SDL versions. They
are not Android GLES screenshots. Existing Phase 13/14 same-ROM/same-WRAM
Compose comparisons remain preserved; the current Compose preview cannot
capture EmulatorView's shader pipeline. See FUNCTIONAL_AUDIT.md.

Warm-cache 180-frame render-only benchmarks are included in logs. Desktop and
QEMU runs varied with shared-host load (roughly 3–13 ms/frame in recorded runs),
so these are cost measurements, not a Brick FPS/audio guarantee. The mask is
cached by dimensions/settings, no per-frame shader-math pass is allocated;
color correction uses a precomputed 32K LUT. All effects are opt-in. Actual
Brick backend blending, sustained speed, audible audio, RTC elapsed time and
power/suspend handling require hardware testing. Prior user-confirmed gameplay,
saves, expansion/zoom, companion and quick-menu results remain hardware facts,
not validation of these new features.

Reproduce the ROM-free display tests with `pocket-functional-test`. For the
normal-input action proof pass the private normal 3.2.3 ROM and a starter state
made by `pocket-rom-test ROM boot-polished-newgame.txt /private/starter.ss`:
`pocket-functional-test ROM /private/starter.ss`. Tests use disposable data
roots. Set SDL_VIDEODRIVER=offscreen, SDL_RENDER_DRIVER=software and
SDL_AUDIODRIVER=dummy for headless integration. ARM tests use the actual NextUI
sysroot under qemu-aarch64-static, not a desktop cross-architecture substitute.

## Earlier verification retained


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

## Phase 10 results (2026-10-22)

Linux and actual NextUI Cortex-A53/AArch64 builds compile. The shared host test verifies upstream mapping passthrough and a forced map-failure memory copy, zero fill, file-position preservation, writeback and ownership. Faults in read/write/sync/truncate/close latch an error; subsequent fallback writes are inhibited. Preferences are isolated by content hash, written through rename, and reject invalid/corrupt content. These are explicit I/O fault tests, not hardware failure claims.

Native and ARM/QEMU diagnostic GB/GBA emulator tests pass both normally and with all save mappings forced to fail. Battery marker persistence, state round trips, controller navigation and 100 companion toggles pass. Quick-menu regressions pass. A private normal 3.2.3 ROM passes the normal-input starter route and provides real individual Guide entry/list/hint/answer captures at 512x384. Decoder tests cover individual selection, reveal steps, close-on-next, nested Back and boss identity changes.

The private-ROM display test passes natively and on ARM/QEMU: exact PPU viewport preservation, display cycling, menu fallback, state cache invalidation, companion/wheel pause, and remembered display mode after actual emulator teardown/reopening. The ARM run also forces save-mapping failure. No physical Brick testing occurred. SD-card failure behavior, sustained hardware speed/audio, RTC and suspend/power remain unverified. No ROM, extracted commercial art or private save/state is distributed.

CI includes pocket-host-test and forced-map-failure diagnostics in both native and ARM jobs. Check Actions separately for remote status. Shared sources are merged, but only features compiled into the native host are claimed.

## Phase 11 results (2026-10-22)

Linux and actual NextUI Cortex-A53/AArch64 builds compile. Generated experience tables, Polished growth rates, party art and prior adapters pass freshness checks. Tests check all six experience curves at every level, lower/upper clamping, level 100, invalid ids/levels, egg suppression and Polished Cyndaquil's medium-slow mapping (Lv5 starts at 135; next level 179; 44 remaining at the actual starter state). Active-party decoding is tested against captured battle WRAM with valid and invalid party indices, including one-member parties.

The supplied normal 3.2.3 ROM passes the ordinary-input fresh-game route, produces actual 512x384 party/summary captures, and reads real Cyndaquil Lv5 HP19/19 EXP135. Paired UI-clock captures assert that only the selected sprite region changes. The empty EXP bar at this starter's level boundary is intentional, not placeholder data. Battle OUT captures are labeled upstream trainer-memory fixtures, not a live trainer playthrough.

Native and ARM/QEMU runtime tests pass game/companion pause/resume, state round trips and quick-menu regressions. The supplied-ROM runtime also passes on both architectures without fixture injection. Physical Brick controls, sustained performance, RTC and suspend remain unverified for this pass. Full Android styling/feature parity remains open in PARITY.md. No commercial ROM or private save/state is shipped.

## Phase 12 results (2026-10-22)

Native Linux and the actual NextUI Cortex-A53/AArch64 builds compile without new compiler warnings. Native and ARM/QEMU decoder tests pass, including HP zero/unknown/tiny-positive/full/overfull and integer-limit handling, original green/amber/red thresholds, pixel bounds, and transparent-center map pulses. The existing generated-table freshness checks pass.

The private normal 3.2.3 ROM passes the normal-input new-game route and produces new map pulse captures at 512x384. Native and ARM/QEMU SDL runtime tests load the genuine starter state, navigate from Party through all intermediate sections to Map and back using individual shoulder events, open Summary, and assert the emulator frame counter remains unchanged throughout. Both resume and pass the save-state round trip. No fixture RAM is injected in this path. Captures show the actual software SDL renderer at 1024x768; logical-resolution companions are exact 2x reductions for inspection.

Native diagnostic GB/GBA regressions pass, including battery-marker persistence, states, section navigation and 100 companion toggles. Native quick-menu and host I/O fault/preference regressions pass. Battle screenshots are separately labeled upstream trainer-WRAM fixtures with artwork/font decoded from the supplied matching ROM; this pass does not claim a newly played trainer encounter. New battle/map captures were visually inspected at 512x384 and the labels adjusted to avoid overlap.

No physical Brick testing occurred in this pass. Audible hardware output, sustained performance, RTC, suspend/power behavior and broad expansion-map coverage remain unverified. Full Android panel styling and feature parity remain unfinished. No ROM, loose extracted game assets, personal save or state is shipped. The screenshots are actual renderer output, not mockups.

The isolated decoder/renderer harness also passes AddressSanitizer and UndefinedBehaviorSanitizer with the supplied ROM and captured fixtures. LeakSanitizer cannot inspect process threads in this environment and was disabled for the successful run; no leak-clean claim is made. This is not a sanitizer run of the full emulator. The manual checks, search tests and website documentation build (`make docs-check`) pass.

## Phase 13 results (2026-10-22)

Native Linux and actual NextUI Cortex-A53/AArch64 builds compile. Native and ARM/QEMU decoder tests cover All/occupied-pocket selection, stable duplicate stacks and all four sort modes, maximum 640-row capacity, malformed pocket counts, selection retention across sort, nested Back and read-only snapshot preservation. Shared text tests verify pagination wrap, clip preservation and no drawing outside the viewport; dimmed-sprite clipping and all previous HP/EXP/battle/Guide predicates pass. Generated styles and prior data/art pass freshness checks.

The supplied normal 3.2.3 ROM passes the ordinary-input new-game test again. Native and ARM/QEMU SDL tests navigate Bag, Dex, Map and Summary while asserting paused core frame counters, return to gameplay and pass save-state round trips. The fresh starter has an empty bag: populated Bag captures are separately labeled upstream memory fixtures. Native diagnostic GB/GBA tests pass battery/state persistence and 100 companion toggles. The isolated decoder/renderer passes ASan/UBSan with leak detection disabled because this environment cannot support LeakSanitizer's thread inspection. This is not a full emulator sanitizer or physical-device test.

The real upstream Compose desktop renderer builds and renders four reference screens from polished_crystal captured WRAM, using the supplied ROM for the Gen 2 font/art/text and its actual hash-selected tables. The four real-fixture captures complete without action failures. The earlier generic preview run had two favorite-interaction failures and is not used as parity evidence. Native Bag and Dex captures and these real-fixture references were inspected at the actual screen size. The comparison corrected the description box's measured height, the OWNED count label, and shared chip framing. It does not establish full-screen parity for the unreviewed screens.

No physical Brick testing occurred. Full Guide/battle/Dex-entry/global-menu visual parity, sustained ARM performance, audio hardware, RTC, suspend/shutdown and power-loss behavior remain unfinished. No ROM, loose extracted commercial art, personal save or state is shipped. Reference and native screenshots are actual render output, explicitly labeled by source and whether memory fixtures were used.


## Phase 14 Guide source fidelity pass

Reference implementation: retained upstream `54d8f4867501feb32b7ee139a5616b7c6301f044`,
`GuideScreen.kt` (GuideUiState, herePage, bossPage, SectionHeading, EntryRow),
`GuideAreas.kt` (generatedWhereIs) and the existing Polished GuideWorld tables.

Completed checks in this pass:

- Native release frontend/test targets and NextUI AArch64 release targets build.
- Native and ARM captured-memory regression tests cover the five supported
  release/faithful fixtures, wild/trainer battle fixtures and 2,776 encounter
  map/method/clock cases, plus 36 time-dependent fish slots.
- Guide checks cover spoiler-safe wild names/icons, hidden item kind-only
  titles until reveal, owned-state changes, stable unique entry keys, four-page
  navigation, one open hint/answer, page visits retaining the open key, and
  progress invalidating the old boss reveal.
- User's normal 3.2.3 ROM is hash matched. A fresh game reads the actual starter,
  level 5 / HP 19/19, its received event flag and the first pending boss without
  fixture injection. Guide captures and long-answer tail/wrap checks pass.
- Native and ARM SDL event tests visit HERE, focus the page selector, choose
  NEXT BOSS, reveal hint and answer, close it, resume and round-trip a state.
- Existing native/ARM runtime regressions retain GB/GBA gameplay, battery
  marker/state handling, quick menu confirmations and 100 companion toggles.
- Native full-resolution expansion checks pass Original/Expand/Zoom, unchanged
  PPU-center pixels, fallback menus, state-load reset and teardown/reopen mode
  persistence. These are emulator checks, additional to user hardware reports.
- AddressSanitizer/UndefinedBehaviorSanitizer captured-memory and ROM-art checks
  pass. LeakSanitizer is disabled because this container cannot inspect `/proc`
  tasks; this is not a leak-check claim.
- All existing generated data/art/style checks pass. Manual parse/link,
  terminology and search checks pass (7 website tests).

Hardware evidence: the user reports that gameplay, expansion/zoom, companion
menus, saves and quick menu work on their actual Brick. The newly adapted Guide
has not yet been retested on the device. No new hardware power, suspend, RTC,
shutdown or sustained-speed claims are made.

Actual Compose-source reference captures completed for the matching upstream
WRAM and ROM: HERE, NEXT BOSS, first boss entry hint/answer, TIPS and WHERE IS.
They confirm the page/section/reveal structure and source spoiler rules. The
comparison exposed solid native dividers; the final pass reuses upstream's
6u/2u dash pattern and 2u stroke via a shared canvas helper. Remaining text
metrics, wrapping and single-screen navigation differences stay documented
in PARITY.md. The package includes labeled source/native comparisons.

The comparison also caught initial row-follow scrolling hiding HERE's area
heading. A newly opened page now starts at its section heading; entry-follow
scrolling starts after controller navigation/reveal. Native/ARM fixture
captures assert that initial page scroll is zero.
