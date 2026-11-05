# Linux parity audit: current source, remaining work and DS one-screen port

Audited application sources, not an assumption of feature equivalence from similar screenshots.
Runtime package reviewed: `brick/libretro-unified` at fb40f3f.

| Reference | Current inspected revision |
| --- | --- |
| lidor30/pokedaisy main | 54f4c66f5ef0857192838d877418c7db96ccfa99 |
| thwonp/pokedaisy-plorp feat/libretro-core | 4ab40fac9c7255853c9c32c1de227836e6f699e2 |
| thwonp/pokedaisy-plorp main | 16f8ec302313a88365dc84769538e9f17d4b082c |
| own unified core/package | fb40f3f1d43b243a4752395493acacaa501a80ae |

Thwonp's libretro implementation is on feat/libretro-core, not his main branch's
standalone SDL host. The unified package pins exactly that libretro revision.
No new heads were found for upstream main or his libretro branch since our build.

## Assessment

The native Linux foundation is working: two libretro cores, one NXRedux entry,
firmware-managed SRAM/states, companion, settings and DS screen controls. It is
not yet a complete DS feature port. The DS quick menu and frame resemble the
GB/GBA UI, but do not provide equivalent data, actions, HUD or settings coverage.

Two emulator cores are an appropriate boundary. The architectural debt is two
versions of portable presentation code: the pinned Thwonp dependency's 3ds/core
and our older 3ds/core plus DS source adapters. They should converge to one source
implementation compiled into both cores; there is no need to combine both
emulators into one .so or add another launcher.

## Presentation and settings comparison

GB/GBA entries below describe the inspected libretro source and supported games,
not a claim that every option is enabled for every ROM or frontend.

| Area | Thwonp GB/GBC/GBA libretro | DS implementation now |
| --- | --- | --- |
| Source foundation | Current pinned implementation | Older portable UI with DS adapters |
| Companion frame | Bottom chips, gear, MORE, contextual tabs | Bottom chips and gear; four fixed content tabs |
| Party | Richer INFO/STATS, IV/EV data where available, shiny presentation | HP/status/level/moves, held item/nature/EXP; no full stat/IV/EV/shiny model |
| Bag | Existing item descriptions, artwork and pocket presentation | Native DS pocket/count/name list; details are essentially the item name |
| Dex | Existing supported-game entry views/data/art | Seen/caught/name list and filters, no full entry pages |
| Map | Existing supported-game map/places/interaction | Decoded Platinum region map, marks/player/cursor; no Fly integration |
| Guide | Supported generated guides | Absent |
| Trainer card | Supported-game card presentation | Absent; a few trainer values appear in Settings |
| Battle companion | Existing supported-game battle data/panes | No live battle snapshot/tab |
| Quick menu | PC/Fly/Repel/Bike/Wild, HUD cursor, companion with availability feedback | Navigation shortcuts to Party/Bag/Map/Dex/Settings plus HUD toggle |
| Quick-menu appearance | Current renderer, appropriate action/ROM icons and submenus | Older Ring/Grid renderer; repeated companion arrow and monitor icon |
| HUD | GBA host: four layouts, three looks, size/parts switches, interactive cursor | Fixed party bar only |
| HUD timing | Free-field detection and settling | Top-only layout check; lacks free-field gating |
| Button settings | Independent quick/companion/HUD cursor assignments | Companion and screen assignments; quick menu is Select + companion; no HUD cursor |
| Tab-bar settings | Configurable selection, MORE for other tabs | Fixed Party/Bag/Map/Dex + Settings |
| Aspect settings | ORIGINAL/EXPAND/ZOOM OUT and AUTO ZOOM IN where supported | DS layout selector; native viewport, no supported expansion/zoom option |
| DS-only preferences | Not applicable | Auto battles, screen selection and pen speed |
| Persistence | Shared settings file, current fields | Separate DS file with atomic writes and bounded legacy migration |
| Saves/resume/speed/shaders | Frontend-owned in this libretro host | Frontend-owned too; appropriate ownership, not a missing companion feature |

Important boundaries: Thwonp's libretro host sets hasHud for GBA, not GB/GBC.
It does not enable its hasDisplayFilter row. Do not mistake every setting in
his standalone Linux/Android implementations for an exposed libretro option.
Matching the settings UI should expose implemented capabilities, not inert rows.

## What upstream ONE SCREEN actually does

1. `nds/pk_nds_field.cpp` supplies portable RAM-based scene/touch rules. Our copy
   of this file and its header is byte-identical to the inspected upstream main.
   It distinguishes touch-only pages, top-only pages, battle panels and fades.
2. `EmulatorEngine.kt` applies temporal policy: touch on after 2 frames, off
   after 6; fading menus wait for black followed by 12 lit frames, or a 90-frame
   timeout. It retains state through black transitions and separates menu/app/
   wide-scene state. Our switcher does not implement that policy.
3. `DsLayoutSettings.kt` describes five arrangements: ONE SCREEN, SIDE BY SIDE,
   LARGE + SMALL, PICTURE IN PICTURE, STACKED. Unknown games fall back to PIP.
   Our AUTO/TOP/BOTTOM/SIDE BY SIDE/STACKED values are not the same semantics.
4. `PokeDaisyActivity.kt` and `DsTouchWindow.kt` keep the game behind a framed,
   dimmed floating touch screen; general two-screen apps can become a side-by-
   side modal over a cached field backdrop. Manual and automatic windows have
   different dismissal rules. Our current AUTO switches the rendered output
   to bottom-only or side-by-side, without this floating composition/backdrop.
5. `DsBattleMenu.kt` draws the game-specific command/move strip and captures
   D-pad/A/B. `NdsBattleTaps.kt` presses the original game-owned touch targets
   for 3 frames, releases for 2, and waits for a stable move panel (4 frames;
   120-frame timeout). It does not invent a different battle engine.
6. `Gen4Reader.kt::readGen4BattleInput` distinguishes ACTION/MOVE/BAG/PARTY/BUSY
   and verifies that a panel is drawn. Our field flags indicate touch selection,
   but the DS companion currently has none of this detailed battle-input model.

The separate native GPU reader, pk_nds_field_gpu.cpp, uses melonDS's 2D state
for text-box information. Our one-colour framebuffer check does not replace all
of that information. The native wrapper also handles preserving the X menu
across touch-driven page exits; this hook is not wired in the current DS core.

## Portability and effort

These are relative engineering assessments from code inspection, not delivery dates.

| Work | Reuse / difficulty |
| --- | --- |
| Scene/touch identification | Already reused for exact supported Platinum; low remaining effort |
| Five layout rectangle calculations | Backend-independent arithmetic; low to moderate |
| Fade/settling policy | Port Kotlin state machine into a tested portable frame controller; moderate |
| Floating touch window and backdrops | Rewrite Compose/View rendering with portable canvas/compositor; moderate |
| Touch and controller mapping | Generalize to the actual touch rectangle for each arrangement; moderate |
| Battle input state and queued taps | Existing authoritative readers/targets/timings; moderate |
| Pixel-faithful Platinum battle strip | Compose drawing must be recreated/adapted in shared C UI; moderate to substantial |
| PC/Fly | Recipes/tables exist in pk_nds_call.cpp, but melonDS RAM/ITCM/JIT adapter must become portable; substantial, separate feature |
| Remaining field actions/data | Reuse app-owned readers/tables with per-feature game validation; substantial |
| True DS EXPAND | melonDS polygon/render hooks, not a layout option; substantial renderer work |

One-screen DS does NOT require an emulator replacement, wide rendering, SDL or
an Android runtime. DSperate already supplies both framebuffers, RAM and touch.
An initial software compositor can preserve native integer-scaled gameplay on
1024x768 while supplying modal/PIP layouts. Animations and blur can follow after
the scene/input semantics are reliable.

Expansion is independent: upstream pk_nds_wide.cpp uses melonDS Polygon/GPU3D
and renderer-specific hooks. The private DSperate projection probe demonstrates
more scenery in one field scene at reduced scale, but still scales all 3D,
has state/culling/gating concerns and does not provide higher-resolution wide
rendering. It is not an upstream expansion port.

## Concrete integration issues

- **Fast-forward/rewind signals:** Thwonp's libretro loop queries
  GET_THROTTLE_STATE to skip expensive expansion during FF and retain the selected
  aspect on rewind. The inspected current NXRedux ma_environment.c does not
  implement it; it does implement GET_FASTFORWARDING. Speed-up itself can work
  while the core misses these optimizations. Add a tested FF fallback in our
  build/integration layer; correct rewind reporting needs a frontend contract.
- **Frameskip signal:** current NXRedux leaves AUDIO_BUFFER_STATUS_CALLBACK
  as TODO. Both cores cannot use that callback's lateness/underrun hints there.
  The DS wrapper disables auto frameskip if registration fails. Do not claim
  performance parity with Thwonp's diatom frontend from core equivalence alone.
- **HUD eligibility:** our DS HUD condition is supported ROM + enabled + top-only
  layout. A top-only battle/menu phase also meets it. Upstream/Thwonp restrict
  the HUD to settled free-field gameplay. This needs an explicit scene gate.
- **Save namespace:** one PDAI collection uses one frontend save tag; identical
  basenames can collide with extension-stripping SRAM formats. Different engine
  settings files fix preference clobbering, not all frontend save-name collisions.
  Packaging/import behavior needs a hardware acceptance test and clear policy.

## Recommended implementation order

1. Unify the DS adapters with Thwonp's current portable presentation source.
   Preserve both cores and app-owned game metadata. No second handwritten table
   set or separate companion renderer should be introduced.
2. Fix the small frontend/HUD eligibility gaps; freeze a capability-based settings
   contract so screenshots are not used as evidence of feature parity.
3. Port the one-screen layout state controller, arrangements, floating compositor
   and touch mapping. Platinum only first; keep manual fallback throughout.
4. Port Gen4 battle-input reading, tap queue and Platinum battle strip. Validate
   action/move/cancel, bag/party/prompts, double-battle fallback and FF timing.
5. Bring over richer HUD, party/Dex/bag/map/guide/card data and actual field jobs.
6. Continue DS renderer expansion separately; add other DS games after Platinum
   behavior is verified, not merely after a game-code table entry exists.

Acceptance before calling DS parity complete: same intended scene/layout/HUD
behavior against upstream screenshots; every exposed setting changes the stated
behavior and survives reload; every visible action works or has an accurate
availability reason; all native face buttons remain usable; state loads and
quick resume clear stale UI/touches; real Brick checks cover audio/frame pacing,
menu/battle transitions, raw save/relaunch, shader/geometry changes and sleep.

This audit changed documentation only. Existing test results establish the
foundation; they do not establish the missing UI/game features or new hardware
compatibility. No new runtime build or full-port completion is claimed.

## Source anchors

- [Upstream DS layout model](https://github.com/lidor30/pokedaisy/blob/54f4c66f5ef0857192838d877418c7db96ccfa99/app/src/main/kotlin/com/pokedaisy/app/companion/DsLayoutSettings.kt)
- [Scene/fade policy](https://github.com/lidor30/pokedaisy/blob/54f4c66f5ef0857192838d877418c7db96ccfa99/app/src/main/kotlin/com/pokedaisy/app/EmulatorEngine.kt#L1180)
- [Floating touch window](https://github.com/lidor30/pokedaisy/blob/54f4c66f5ef0857192838d877418c7db96ccfa99/app/src/main/kotlin/com/pokedaisy/app/companion/ui/DsTouchWindow.kt)
- [Controller battle presentation](https://github.com/lidor30/pokedaisy/blob/54f4c66f5ef0857192838d877418c7db96ccfa99/app/src/main/kotlin/com/pokedaisy/app/companion/ui/DsBattleMenu.kt)
- [Frame-based touch queue](https://github.com/lidor30/pokedaisy/blob/54f4c66f5ef0857192838d877418c7db96ccfa99/app/src/main/kotlin/com/pokedaisy/app/NdsBattleTaps.kt)
- [Portable field/touch rules](https://github.com/lidor30/pokedaisy/blob/54f4c66f5ef0857192838d877418c7db96ccfa99/app/src/main/cpp/nds/pk_nds_field.cpp)
- [Current Thwonp settings renderer](https://github.com/thwonp/pokedaisy-plorp/blob/4ab40fac9c7255853c9c32c1de227836e6f699e2/3ds/core/pd_ui_settings.c)
- [Thwonp libretro loop and capabilities](https://github.com/thwonp/pokedaisy-plorp/blob/4ab40fac9c7255853c9c32c1de227836e6f699e2/libretro/pd_libretro.c)
- [Current NXRedux environment callbacks](https://github.com/nx-redux/nx-handheld/blob/main/workspace/all/minarch/ma_environment.c)
