# Polished Crystal Android parity boundary — Phase 13

This compares the native port with the **Polished Crystal path** at upstream
`54d8f4867501feb32b7ee139a5616b7c6301f044`, rather than with Android features
that only other games support. All displayed game art/font/text is read from the
user's matching cart. Full visual and behavioral parity is still unfinished.

| Area | Current native behavior | Remaining work |
| --- | --- | --- |
| Gameplay | mGBA, integer scaling, audio, input, saves/states, fast-forward | Sustained hardware performance, RTC, suspend/power verification |
| Identity | Upstream's 16 exact normal/faithful/debug build hashes | Import and test future builds |
| Font | Original ROM font and Gen2Art mapping, crisp logical 2x glyphs | Remaining per-widget spacing/animations |
| Party | Six slots, rounded upstream frames, original HP lettering, larger ROM icons, selected-icon bobbing, gender/status/held markers and egg handling | Remaining fine layout details; upstream Gen 2 EXP bar is on Summary |
| Summary | Left-info/right-moves hierarchy, held item, EXP progress/NEXT/MAX and HP bars, descriptions, type matchups, party cycling | Remaining styling/animation; Android's Polished decoder does not supply IV/EV stats or LEARN data |
| Bag | Upstream magenta stripes, folder tabs, All/occupied pockets, cursor, framed list and inline complete descriptions; all quantities/money and four read-only sorts with stack retention | Favorites and fine layout details; Gen 2 supplies no item icons; upstream FieldItems has no Polished USE/GIVE/TAKE implementation |
| Battle | INFO effectiveness labels; upstream power/type/risk ranking; live foe item/stats/moves and type-matchup groups; browsable team with unseen slots; battle-scoped seen tracking and exact active-party OUT marker and original-style HP bars | Remaining Android panel layout; Gen 2 supplies no announced-next-foe index; upstream GBA command controls are not Polished controls |
| Dex | Upstream counts/filter + icon-list layout, original caught/seen marks and muted unseen entries; real flags/order, four filters, front sprites, descriptions, dimensions/base stats, species abilities | Entry-page layout, held-button navigation/repeat and further styling; upstream PokedexScreen has no name-search feature; upstream's Polished dex has no evolution table |
| Map | All three ROM maps, palettes/flips, live location/player, pulsing landmark cursor, regions, Places | Precise location refinements; Polished Fly is not provided by upstream FieldItems |
| Guide HERE | Live map/time encounter lists, all eight methods, badge-adjusted levels, caught marks; area finds and progress marks | More exact Android layout, per-entry icons and encounter-detail interactions |
| Guide NEXT BOSS | Progress-dependent order, League/rematch rules, normal/faithful teams, type hints, individual Pokemon hint/answer entries, moves/held-item answers, own-best-level comparison | Exact Android panel styling |
| Guide TIPS / WHERE IS | All seven upstream tips, hints/answers; HM/key/gift-Pokemon lists with deduplication, locations and owned marks | Android's exact panel layout and further list ergonomics |
| Quick menu | L3 ring; original Android icons; paused integer-scaled gameplay behind it; state confirmation, FF and companion | More actions only when their game-specific implementations are ported; physical button/overlay retest |
| States/options | Existing host menu/state/FF controls | Multi-slot screenshot browser, broader options and remaining lifecycle integration |

Guide definitions are generated from `GuidePolishedCrystal*.kt`; the original
Android guide marks itself unverified and may contain source-data mistakes.
Guide progress is read-only: no event, badge, bag or party writes are introduced.
There are no invented memory offsets or device-side downloaded assets.

No DS, achievements, cloud or new ROM-hack support was added. GBA gameplay and
retail companion support remain as before; this does not add Seaglass decoding.

## Expansion and shortcuts

L3 opens the quick-menu ring. R3 cycles Original / Expand / Zoom, with a Menu > Display fallback. The existing `pk_wide_gb.c` renderer now has a native-only adapter for normal Polished Crystal 3.2.3, using official release symbols and the game's actual decompressed metatile/attribute banks. Original pixels are preserved. Menus/dialogue/battles return to the normal viewport. Other builds do not receive guessed addresses.

Remaining expansion work: unspawned/connected-map NPC templates; broad outdoor, map-connection, weather/lighting and battle-transition verification; physical performance and button retest. The initial real-ROM verification covers Elm's laboratory, normal starter dialogue, display cycling, paused companion/wheel and state reset.

## Next Polished work, before GBA companion expansion

1. Hardware validation and wider exploration of this renderer, including connected maps.
2. Remaining battle panel layout and foe-selection ergonomics.
3. Remaining Dex entry/Guide layouts, Bag favorites and shared menu composition.
4. Remaining exact widget styling and per-entry Guide icons.
5. Multi-slot state browser and broader lifecycle/RTC/suspend verification.

These are still open; Phase 13 does not claim full Android parity. GBA work is deferred.

Phase 11 remembers display mode per ROM content identity and reuses upstream's save mapping fallback with native I/O failure reporting. A new shared host module owns those policies; Polished readers/renderers remain independent. Upstream 1.3.1 source history is merged, but Android feature breadth is not implied by that merge.

Phase 11 corrects two earlier checklist assumptions: upstream Gen 2 uses its EXP bar in Summary, and its Dex list has no name search. Those are not missing Party/Dex features to invent. The native summary now uses the exact upstream experience tables and Polished rate mapping. Shared experience progress remains independent of game memory and rendering.

Phase 12 audits ActiveTables.activeItemSpriteDir (the `crystal` predicate includes all Gen 2), Gen2Reader/Telemetry's absent enemyNext, and PartyScreen's SpeciesIcon first-frame path. Item icons, automatic announced-next-foe selection and two-frame Gen 2 party animation are not missing upstream Polished features. Battle health and MapScreen's four-corner pulse are now adapted through palette-driven, emulator-independent pixel widgets. Remaining lists above describe actual native gaps rather than features belonging to other games.

## Direct visual comparisons

Phase 13 extends the upstream desktop preview with matching-ROM Gen 2 art and a captured-Polished-WRAM adapter. Both app UIs can now render the same fixture through their own real decoders, using the same ROM font/icons/text. The reference harness sets and restores the activity's in-game window palette during companion composition. This removes two preview fallbacks that previously hid the actual Game Boy style.

Bag and Dex list have now received this comparison. The Brick keeps top shoulder tabs and control hints, while Android has bottom tabs plus MORE/settings. Dex number/name rows wrap on Brick; long item descriptions page rather than making the whole tab too short. Those adaptations are documented rather than described as pixel-identical parity. Party/Summary, Guide, battle panels, Dex entry pages and global menus still need the same full side-by-side pass. Performance, RTC, suspend/power and state-browser work remain separate unfinished milestones before GBA.
