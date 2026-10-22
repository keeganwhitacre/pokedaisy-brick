# Polished Crystal Android parity boundary — Phase 11

This compares the native port with the **Polished Crystal path** at upstream
`54d8f4867501feb32b7ee139a5616b7c6301f044`, rather than with Android features
that only other games support. All displayed game art/font/text is read from the
user's matching cart. Full visual and behavioral parity is still unfinished.

| Area | Current native behavior | Remaining work |
| --- | --- | --- |
| Gameplay | mGBA, integer scaling, audio, input, saves/states, fast-forward | Sustained hardware performance, RTC, suspend/power verification |
| Identity | Upstream's 16 exact normal/faithful/debug build hashes | Import and test future builds |
| Font | Original ROM font and Gen2Art mapping, crisp logical 2x glyphs | Remaining per-widget spacing/animations |
| Party | Six slots, rounded upstream frames, original HP lettering, larger ROM icons, selected-icon bobbing, gender/status/held markers and egg handling | Remaining icon-frame animation and fine layout details; upstream Gen 2 EXP bar is on Summary |
| Summary | Left-info/right-moves hierarchy, held item, EXP progress/NEXT/MAX and HP bars, descriptions, type matchups, party cycling | Remaining styling/animation; Android's Polished decoder does not supply IV/EV stats or LEARN data |
| Bag | All real quantities/pockets/money, matching-ROM descriptions | Further item-icon/layout parity; upstream FieldItems has no Polished USE/GIVE/TAKE implementation |
| Battle | INFO effectiveness labels; upstream power/type/risk ranking; live foe item/stats/moves and type-matchup groups; browsable team with unseen slots; battle-scoped seen tracking and exact active-party OUT marker | Remaining Android layout/HP-bar details, automatic next-foe selection; upstream GBA command controls are not Polished controls |
| Dex | Real flags/order, four upstream filters, front sprites, descriptions, dimensions/base stats, species abilities | Remaining navigation/styling; upstream PokedexScreen has no name-search feature; upstream's Polished dex has no evolution table |
| Map | All three ROM maps, palettes/flips, live location/player, cursor, regions, Places | Cursor blink and precise location refinements; Polished Fly is not provided by upstream FieldItems |
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
2. Remaining battle layout and foe-selection refinements.
3. Remaining Dex/Guide layout details.
4. Remaining icon-frame animation, item icons and exact widget styling.
5. Multi-slot state browser and broader lifecycle/RTC/suspend verification.

These are still open; Phase 11 does not claim full Android parity. GBA work is deferred.

Phase 11 remembers display mode per ROM content identity and reuses upstream's save mapping fallback with native I/O failure reporting. A new shared host module owns those policies; Polished readers/renderers remain independent. Upstream 1.3.1 source history is merged, but Android feature breadth is not implied by that merge.

Phase 11 corrects two earlier checklist assumptions: upstream Gen 2 uses its EXP bar in Summary, and its Dex list has no name search. Those are not missing Party/Dex features to invent. The native summary now uses the exact upstream experience tables and Polished rate mapping. Shared experience progress remains independent of game memory and rendering.
