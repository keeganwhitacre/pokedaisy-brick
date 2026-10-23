# Phase 15 functional audit

Reference: checked-in Android source at upstream
`54d8f4867501feb32b7ee139a5616b7c6301f044`. The Android implementation remains
intact. This table separates **1: existing Polished functionality**, **2: other
game functionality requiring adaptation**, and **3: downstream additions**.
It is a port boundary, not a claim that Phase 15 completes every feature.

| Feature / category | Android implementation and applicability | Native status and faithful next step |
| --- | --- | --- |
| Bag browsing, sorting, descriptions (1) | ItemsScreen, Gen2Reader, original Polished ROM strings | Existing native implementation retained |
| Bag favorites (1) | FavoriteItems, ItemsScreen; per-ROM persisted ID set, Favorites tab, stars first in Default order | Missing; use a portable ID set and persistent host storage, preserve other sort semantics |
| Field item Use/Give/Take (2) | ItemUse -> FieldItems.fieldItemsByKind; no Polished entry; supported GBA layouts use verified game functions/jobs | Missing; cannot call this Polished parity. Source-verified controller adapter needed for Polished |
| Healing item + party target on Polished (3) | No Android Polished FieldItems adapter | Deferred beyond menu-opening foundation; target/completion and restrictions must be verified first |
| Original in-game Bag shortcut (3) | New Polished controller sequence; not Android FieldItems | Implemented for normal 3.2.3; see ACTIONS.md |
| Battle actions / moves (1) | BattleInput, BattleInputController, Gen2Reader.readGen2BattleInput; Gen 2 2x2 actions and move column; waiting cursor tile checked | Phase 17 ports normal 3.2.3 controls; genuine native/ARM Lyra-battle PP, Bag, Back and trainer Run-refusal proofs pass |
| Battle party switching (1) | Activity.switchAddrsFor -> Gen2PartySwitch, readGen2PartyMenu; explicit Polished submenu geometry and party identity | Phase 17 implements optional/forced switching with identity lookup and item-target exclusion; capture/model tests pass; genuine multi-member execution still pending |
| PC / Fly / Repel / Bike / Rod / wild shortcuts (2) | Activity.quickActions.offered gates capabilities; FieldItems has no Polished adapter; GbField.forGame accepts retail Crystal, not Polished | Not offered or simulated. These need individual verified Polished adapters; cannot reuse retail addresses or GBA item IDs |
| Ring state save/load, FF, companion (1) | QuickMenu, QuickMenuSettings, engine input/state callbacks | Existing four-action ring retained; configurable actions/grid style still missing |
| LCD, SCANLINES, GRID (1) | ScreenShaders + EmulatorView; texel-relative darkening, integer prescale; strengths .2/.35/.5 | New portable equations + cached SDL alpha mask, .2/.349/.502 after 8-bit quantization; applies to normal and expanded game image |
| GBA COLORS (1) | ScreenShaders.GBA_COLOR, gamma/matrix/gamma before filters; switch also applies while GB is running | New 32K RGB555 lookup; preserves source coefficients/defaults; no invented separate GBC preset |
| LCD PAPER, CRT, UNLIT SCREEN (1) | ScreenShaders: multi-sample/noise/paper, Gaussian/mask and shadow passes | Missing; not relabeled LCD/scanlines. SDK has EGL/GLESv2, but a supported accelerated runtime path/performance is not established |
| xBRZ, xBR, ScaleFX (1) | Original smoothing passes; ScaleFX uses intermediate passes/formats | Missing. Translate or reuse actual passes, do not substitute SDL linear smoothing and call it xBR |
| ORIGINAL, STRETCH, EXPAND (1) | CompanionSettings.GameAspect, EmulatorView + existing shared pk_wide code | Existing integer Original/Expand/Zoom retained; new unfiltered STRETCH. Filter + stretch rejected pending faithful prescale/resample path |
| Settings hierarchy / controller navigation (1) | CompanionSettingsScreen, screen and shader pages, Prefs | New controller SCREEN / SHADERS subset in Game > Settings. Names/default grid retained; combined host menu is an explicit hierarchy/layout adaptation, not full Compose parity |
| Display persistence (3 adaptation) | Android SharedPreferences; display prefs are global by default | Native choices persist per content SHA-1 in display/*.fx; original *.mode and settings.ini retained |
| Button mapping, hotkeys, FF modes/scene speeds (1) | GbaInput, Hotkeys, CompanionSettingsScreen, EmulatorEngine | Existing Brick mappings and 2x toggle retained. Configurable mappings, source FF modes and scene speed controls still missing |
| Companion visibility / shaders (1) | Screens, light companion and CompanionColors; companion shaders default on | X single-screen navigation retained. Companion/status-bar shaders, light companion and Android multi-display behavior missing |
| Multi-slot state browser / previews / undo (1) | SaveStates, StateSlots, StatesScreen; thumbnails and resume/undo files | Existing single state and confirmations retained. No state-location migration; add browser compatibly later |
| RTC, resume, shutdown and NextUI (platform adaptation) | Android lifecycle, engine resume and mGBA RTC/state serialization | Existing saves/state/launcher retained; clock loading tested, elapsed RTC and real power integration still need device verification |

The original Kotlin/JNI emulator integration drives its normal game through a
scripted input controller and, for some other-game field jobs, verified native
routines. Its Android threading, Compose state, SharedPreferences and GL surface
cannot run unchanged in this host. Pure action observations/dispatch and display
math are separated from SDL; mGBA, upstream save VFile and expanded renderer
continue to be reused directly. No architectural rewrite was undertaken.

## Display verification limits

The SDL implementation preserves original texel-space equations at the integer
sizes used by Brick, with 8-bit blend rounding. The equations are tested against
a floating-point reference across GB/GBA/expanded geometry. GBA COLORS uses the
same gamma and column-major matrix, indexed by the core's expanded RGB555.
Warm-cache software SDL costs are recorded in verification logs; effects are
opt-in and defaults retain the existing opaque renderer. Failed mask creation
is latched until a setting change to avoid allocating every frame.

The existing Android UI-preview system renders Compose companion screens; it
is not an EmulatorView/Android GLES capture harness. Phase 13/14 same-ROM,
same-WRAM comparisons are preserved. Phase 15 game-effect captures are actual
native emulator frames, not Android-rendered shader comparisons. Full GL image
diff and physical Brick speed/audio remain unverified. No lookalike replacement
is enabled for an unavailable upstream effect.
