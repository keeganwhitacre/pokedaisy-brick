# Polished Crystal Android parity boundary — Phase 9

This compares the native port with the **Polished Crystal path** at upstream
`a64c9172828bb2e3824d5211655091e27f7ff73c`, rather than with Android features
that only other games support. All displayed game art/font/text is read from the
user's matching cart. Full visual and behavioral parity is still unfinished.

| Area | Current native behavior | Remaining work |
| --- | --- | --- |
| Gameplay | mGBA, integer scaling, audio, input, saves/states, fast-forward | Sustained hardware performance, RTC, suspend/power verification |
| Identity | Upstream's 16 exact normal/faithful/debug build hashes | Import and test future builds |
| Font | Original ROM font and Gen2Art mapping, crisp logical 2x glyphs | Remaining per-widget spacing/animations |
| Party | Six slots, icons, gender, HP, status, held marker, egg handling | Icon bobbing, EXP bar and exact slot-frame details |
| Summary | Left-info/right-moves hierarchy, held item, EXP, descriptions, type matchups, party cycling | Remaining styling/animation; Android's Polished decoder does not supply IV/EV stats or LEARN data |
| Bag | All real quantities/pockets/money, matching-ROM descriptions | Further item-icon/layout parity; upstream FieldItems has no Polished USE/GIVE/TAKE implementation |
| Battle | INFO effectiveness labels; upstream power/type/risk ranking; live foe item/stats/moves and type-matchup groups; browsable team with unseen slots; battle-scoped seen tracking | Remaining Android layout/HP-bar details, active-party marker and automatic next-foe selection; upstream GBA command controls are not Polished controls |
| Dex | Real flags/order, four upstream filters, front sprites, descriptions, dimensions/base stats, species abilities | Search and remaining navigation/styling; upstream's Polished dex has no evolution table |
| Map | All three ROM maps, palettes/flips, live location/player, cursor, regions, Places | Cursor blink and precise location refinements; Polished Fly is not provided by upstream FieldItems |
| Guide HERE | Live map/time encounter lists, all eight methods, badge-adjusted levels, caught marks; area finds and progress marks | More exact Android layout, per-entry icons and encounter-detail interactions |
| Guide NEXT BOSS | Progress-dependent order, League/rematch rules, normal/faithful teams, type hints, moves/held-item answers, own-best-level comparison | Per-Pokemon reveal instead of whole-team reveal |
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
2. Remaining battle layout/active-party indication and foe-selection refinements.
3. Dex search and Guide individual reveals.
4. Party EXP bar/icon animation, item icons and exact widget styling.
5. Multi-slot state browser and broader lifecycle/RTC/suspend verification.

These are still open; Phase 9 does not claim full Android parity. GBA work is deferred.
