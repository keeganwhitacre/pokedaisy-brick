# Polished Crystal Android parity boundary — Phase 20

Phase 20 retains the existing controller mappings and clarifies the Summary
and Bag prompts. Settings now expose per-ROM 2x/3x/4x fast-forward and Android's
ON COMPANION choice for the currently supported LCD/SCANLINES/color effects.
The battle background extension is a downstream option, not additional PPU
rendering. It samples matching light corners/border of the current battle frame,
fills only outside the original game image, and safely falls back to black.
Fly remains unavailable; see FLY_INVESTIGATION.md for verified game rules and
the controller-driven implementation path. Fast-forward music is out of scope.

Phase 19 adapts SaveStates.kt and StatesScreen/SlotCard: ten numbered slots,
timestamps, thumbnails and separate resume storage. Brick uses two columns,
D-pad selection and Save/Load mode with confirmation, retaining OPTION styling.
Unlike Android's separate thumbnail file, the mGBA extended state contains
its own image and identity/integrity metadata, replaced atomically. Existing
slot-0 files remain compatible. Undo Save/Undo Load are still missing. These
are explicit differences, not a claim of complete StatesScreen parity.

Linux usability additions: a sole Tools ROM launches directly; --library keeps
the picker. AUTO RESUME ON closes with one action, with retry/keep-playing on
recovery write failure. The existing game-list wrapper, backup/save paths,
power adapter and separate recovery validation remain. See SAVE_STATES.md.

This compares the native port with the **Polished Crystal path** at upstream
`54d8f4867501feb32b7ee139a5616b7c6301f044`, rather than with Android features
that only other games support. All displayed game art/font/text is read from the
user's matching cart. Full visual and behavioral parity is still unfinished.

| Area | Current native behavior | Remaining work |
| --- | --- | --- |
| Gameplay | mGBA, integer scaling, audio, input, saves/states, fast-forward | Base gameplay, saves, expansion/zoom, companion and quick menu confirmed on Brick by the user; sustained performance, RTC and suspend/power verification remain |
| Identity | Upstream's 16 exact normal/faithful/debug build hashes | Import and test future builds |
| Font | Original ROM font and Gen2Art mapping, crisp logical 2x glyphs | Remaining per-widget spacing/animations |
| Party | Six slots, rounded upstream frames, original HP lettering, larger ROM icons, selected-icon bobbing, gender/status/held markers and egg handling | Remaining fine layout details; upstream Gen 2 EXP bar is on Summary |
| Summary | Left-info/right-moves hierarchy, held item, EXP progress/NEXT/MAX and HP bars, descriptions, type matchups, party cycling | Remaining styling/animation; Android's Polished decoder does not supply IV/EV stats or LEARN data |
| Bag | Upstream magenta stripes, folder tabs, All/occupied pockets, cursor, framed list and inline complete descriptions; all quantities/money and four read-only sorts with stack retention | Favorites and fine layout details; Gen 2 supplies no item icons; upstream FieldItems has no Polished USE/GIVE/TAKE implementation |
| Battle | Existing INFO/advice/team features plus source-derived action/move controls, original battle Bag/Run/Back and identity-checked switching on normal 3.2.3 | Multi-member real-ROM switch validation; other builds; remaining Android layout/animation, processing indicator and held-frame switching; Gen 2 supplies no announced-next-foe index |
| Dex | Upstream counts/filter + icon-list layout, original caught/seen marks and muted unseen entries; real flags/order, four filters, front sprites, descriptions, dimensions/base stats, species abilities | Entry-page layout, held-button navigation/repeat and further styling; upstream PokedexScreen has no name-search feature; upstream's Polished dex has no evolution table |
| Map | All three ROM maps, palettes/flips, live location/player, pulsing landmark cursor, regions, Places | Precise location refinements; Polished Fly is not provided by upstream FieldItems |
| Guide HERE | Android left page selector and inline sections for every present encounter method, PEOPLE and ITEMS; caught counts/balls, owned-last area rows, time/badge levels, encounter icons and spoiler-safe reveals | First-use guide notice, small-text sizing and fine layout/scroll-animation differences |
| Guide NEXT BOSS | Progress-dependent order, League/rematch rules, normal/faithful teams, type hints, individual Pokemon hint/answer entries, moves/held-item answers, own-best-level comparison | Shared OPTION frames and inline per-Pokémon entries now adapted; fine text metrics and controller scrolling differences remain |
| Guide TIPS / WHERE IS | All seven upstream tips, hints/answers; HM/key/gift-Pokemon lists with deduplication, locations and owned marks | Inline hint/answer rows with source page/category hierarchy now adapted; fine text metrics and first-use guide notice remain |
| Quick menu | L3 ring; original Android icons; paused integer-scaled gameplay behind it; state confirmation, FF and companion | More actions only when their game-specific implementations are ported; User confirms quick menu on Brick; new UI changes still require hardware retesting |
| States/options | Ten preview slots; per-ROM 2x/3x/4x FF, ON COMPANION, LCD/SCANLINES/GRID/GBA COLORS, unfiltered STRETCH, AUTO ZOOM IN, AUTO RESUME and battle background | Full Android hierarchy, remaining effects/mapping/scene-speed options and Undo state actions |
| NextUI lifecycle | Foreground power adapter, real suspend helper, pause/audio/input restoration, separate atomic recovery and direct-launch pak | Physical sleep/wake/shutdown/RTC/audio testing; firmware monitor/LED/haptic differences |

Guide definitions are generated from `GuidePolishedCrystal*.kt`; the original
Android guide marks itself unverified and may contain source-data mistakes.
Guide progress is read-only: no event, badge, bag or party writes are introduced.
There are no invented memory offsets or device-side downloaded assets.

No DS, achievements, cloud or new ROM-hack support was added. GBA gameplay and
retail companion support remain as before; this does not add Seaglass decoding.

## Expansion and shortcuts

L3 opens the quick-menu ring. R3 cycles Original / Expand / Zoom, with a Menu > Display fallback. The existing `pk_wide_gb.c` renderer has a native-only adapter for normal Polished Crystal 3.2.3, using official release symbols and the game's actual decompressed metatile/attribute banks. Original pixels are preserved. Ready matched dialogue stays expanded unless AUTO ZOOM IN is ON; menus, battles and unmatched frames return to the original viewport. Other builds do not receive guessed addresses.

Phase 18 adds ordinary unspawned/connected-map NPC templates; see EXPANSION.md for supported objects and conservative omissions. Broad outdoor, weather/lighting and battle-transition verification remain. User confirms expansion/zoom and quick-menu operation on Brick; broader performance, lifecycle and new-UI retests remain. Real-ROM coverage now includes lab dialogue, New Bark Town and a normal Route29 connection crossing.

## Next Polished work, before GBA companion expansion

1. Hardware validation and wider exploration of this renderer, including connected maps.
2. Remaining battle panel layout and foe-selection ergonomics.
3. Remaining Dex entry layouts, Guide notice and shared menu composition.
4. Remaining exact widget styling and small-text metrics.
5. Multi-slot state browser and physical lifecycle/RTC/suspend verification.

These are still open; Phase 14 does not claim full Android parity. GBA work is deferred.

## Phase 16: native handheld integration

Phase 17 adds the existing Android Polished battle interaction path; see
INTERACTIONS.md for supported ROM identity, controls and validation limits.
The read-only information pages remain available. Native and ARM/QEMU genuine
Lyra-battle tests prove second-move PP consumption and companion return, Bag,
Back and game-owned trainer Run refusal. Switching is implemented from the
upstream state machine, but genuine multi-member/forced-switch runtime testing
is still pending beyond captured observations and model tests. No Polished
field-item use/give/take or healing-target extension was added.

Android WideCamera/Prefs already supports disabling AUTO ZOOM IN. The host now
accepts ready MAP/TEXT frames from the unchanged shared renderer. Default OFF
is an explicit user-requested difference from Android's ON. Android's timed
camera tween is still absent; safe menu/battle fallback is retained.

The Phase 16 audit verified the loaded 34-byte object layout against actual
release source. Phase 18 now adapts ordinary unspawned/connected-map objects,
with a real New Bark/Route29 crossing test. Special objects, missing palettes
and broader connected-map conditions remain limited; see EXPANSION.md.

Recovery and power are Linux/NextUI adaptations, preserving the existing
mGBA frontend. Recovery validates state and current battery content and never
contains SRAM; manual states and battery-save backups remain separate. See
LIFECYCLE.md for provenance, bypasses and precise firmware differences. All
physical sleep/wake and elapsed RTC conclusions still await Brick testing.

Phase 11 remembers display mode per ROM content identity and reuses upstream's save mapping fallback with native I/O failure reporting. A new shared host module owns those policies; Polished readers/renderers remain independent. Upstream 1.3.1 source history is merged, but Android feature breadth is not implied by that merge.

Phase 11 corrects two earlier checklist assumptions: upstream Gen 2 uses its EXP bar in Summary, and its Dex list has no name search. Those are not missing Party/Dex features to invent. The native summary now uses the exact upstream experience tables and Polished rate mapping. Shared experience progress remains independent of game memory and rendering.

Phase 12 audits ActiveTables.activeItemSpriteDir (the `crystal` predicate includes all Gen 2), Gen2Reader/Telemetry's absent enemyNext, and PartyScreen's SpeciesIcon first-frame path. Item icons, automatic announced-next-foe selection and two-frame Gen 2 party animation are not missing upstream Polished features. Battle health and MapScreen's four-corner pulse are now adapted through palette-driven, emulator-independent pixel widgets. Remaining lists above describe actual native gaps rather than features belonging to other games.

## Direct visual comparisons

Phase 13 extends the upstream desktop preview with matching-ROM Gen 2 art and a captured-Polished-WRAM adapter. Both app UIs can now render the same fixture through their own real decoders, using the same ROM font/icons/text. The reference harness sets and restores the activity's in-game window palette during companion composition. This removes two preview fallbacks that previously hid the actual Game Boy style.

Bag and Dex list have now received this comparison. The Brick keeps top shoulder tabs and control hints, while Android has bottom tabs plus MORE/settings. Dex number/name rows wrap on Brick; long item descriptions page rather than making the whole tab too short. Those adaptations are documented rather than described as pixel-identical parity. Party/Summary, battle panels, Dex entry pages and global menus still need the same full side-by-side pass. Guide reference capture support was added in Phase 14. Performance, RTC, suspend/power and state-browser work remain separate unfinished milestones before GBA.

## Phase 14: Guide source translation

`GuideScreen.kt` is the reference for page/section order, `GuideUiState.tap`,
`herePage`, `bossPage`, `SectionHeading` and `EntryRow`. `GuideAreas.kt`
`generatedWhereIs` supplies category and location wording. Native HERE now
combines all present encounter methods and area finds; NEXT BOSS lists each
remaining team as sections rather than a separate team modal. TIPS and WHERE
IS reveal inline. Unseen wild names/icons and uncollected hidden-item names
remain concealed as on Android. Only one entry can be open; its stable key
survives a tab/page visit and becomes invalid if its map/boss disappears.

`gen_style.py` imports exact Guide method titles/order and page labels and
records the UI input hash. Its checks fail if upstream changes those source
shapes. The portable canvas now handles explicit paragraph breaks and includes
Pixel Operator's actual middle-dot glyph as the existing font fallback.
No game memory, save handling, expansion or SDL hardware policy was changed.

Unavoidable current adaptations: the companion retains shoulder tabs above
its content on the single Brick screen. The left column approximates Android's
34/66 split on a 512x384 logical canvas. Inline title/detail/tag lines wrap to
keep the 16-pixel ROM font readable rather than reproducing Compose's smaller
secondary text. Left focuses pages, Right/A returns to entries, Y advances a
page, A reveals, and Select scrolls an answer taller than the viewport. B
closes a current reveal and otherwise returns to gameplay. Scrolling is
controller-driven rather than Compose touch/kinetic scrolling. The original
first-use GuideNotice/persisted acceptance is still missing and explicitly
remains a gap. The source guide's AI-assisted/unverified limitation still
applies; this port does not validate or replace its data.

Phase 14's actual same-fixture Compose renders cover HERE, NEXT BOSS and its
hint/answer states, TIPS and WHERE IS. Source/native inspection corrected the
row and page-selector dividers to GbaMenu's 6u/2u dash geometry. The full-sized
ROM text makes some native details wrap below the title rather than sit on the
same line as Android. That difference remains explicit, with comparisons in
the package; this is not a pixel-identical claim.

## Phase 15: functionality before further styling

See FUNCTIONAL_AUDIT.md for categories 1/2/3 and ACTIONS.md for exact source,
checks and limits. Start in the companion Bag now opens the real running
game's Bag through normal controller input on verified normal 3.2.3. It does
not select/use the companion item, heal a Pokémon, or change inventory RAM.
Other companion builds reject the shortcut; their existing features remain.

New display choices are adaptations of ScreenShaders, not unrelated filters.
Game > Settings exposes the supported subset with persistent per-ROM choices.
LCD PAPER, CRT, UNLIT, smoothing and the stretch/filter
combination remain gaps. Settings use the existing host menu and source names;
full Android menu composition is not claimed. Existing quick-menu appearance,
state handling, save paths and expansion remain intact.

Functional priorities now include Android Polished battle commands/switching,
Broader original settings/quick controls and state previews.

## Phase 17 follow-up: Bag favorites

Translated the existing Polished Crystal functionality in `FavoriteItems.kt`,
`Prefs.favoriteItemsFor/setFavoriteItemsFor`, and `ItemsScreen.kt` (category and
sort rules, description-bar favorite action). X in an open Bag description
stars/unstars its item id. Duplicate stacks share the star. Default order is a
stable favorites-first partition; Name, Type and Count retain their comparators.
Favorites appears after All only while a starred item is held. Removing the
last one returns to All; temporarily absent item ids remain stored.

Linux preferences live separately in `favorites/<ROM-SHA1>.txt`, with bounded
parsing and atomic replacement. Android uses CRC32-keyed SharedPreferences;
Linux uses the existing SHA-1 identity and reports storage failure. No SRAM,
emulator state, original Bag order, or game-memory changes. The displayed star
is the original `PixelIcons.star`, using `OptionColors.favorite` light color.
The physical X shortcut adapts Android's description-bar star button. Outside
an open Bag description, X retains its game/companion toggle.

Native decoder/controller tests and preference tests pass, including duplicate
stacks, tab removal, missing/corrupt files and the complete 16-bit item-id range.
Native captures were inspected using upstream RAM snapshots and ROM font.
The actual NextUI ARM build, ARM/QEMU decoder/preference tests, and native plus
ARM/QEMU live frontend tests also pass. The live test earns the Potion through
the original Lyra battle and aide gift, then stars, relaunches and unstars it
without unpausing the companion. Physical Brick validation remains pending.
The Compose preview was retried, but Gradle download fails with DNS
`UnknownHostException: services.gradle.org`; no fresh Android comparison is
claimed. Matching Gen 2 favorite/tab preview shots are now defined for an
environment with the preview dependencies available.
This does not add Polished Crystal item Use/Give/Take, which the
upstream Android item-action backend does not implement.
Healing-item targeting is a separately identified downstream extension that
requires additional verified completion/context predicates. Physical Brick
validation of Phase 15 remains outstanding.
