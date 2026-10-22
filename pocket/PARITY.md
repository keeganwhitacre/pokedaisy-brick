# Polished Crystal Android parity boundary — Phase 5

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
| Battle | Actual active battlers, types, moves/PP, trainer-party count | INFO/foe-details/team and suggestions panes, move effectiveness labels and layout parity; upstream's GBA command controls are not Polished controls |
| Dex | Real flags/order, front sprites, descriptions, dimensions/base stats, species abilities | Filters/search and remaining navigation/styling; upstream's Polished dex has no evolution table |
| Map | All three ROM maps, palettes/flips, live location/player, cursor, regions, Places | Cursor blink and precise location refinements; Polished Fly is not provided by upstream FieldItems |
| Guide HERE | Live map/time encounter lists, all eight methods, badge-adjusted levels, caught marks; area finds and progress marks | More exact Android layout, per-entry icons and encounter-detail interactions |
| Guide NEXT BOSS | Progress-dependent order, League/rematch rules, normal/faithful teams, type hints, moves/held-item answers | Per-Pokemon reveal instead of whole-team reveal; own-best-level comparison |
| Guide TIPS / WHERE IS | All seven upstream tips, hints/answers; HM/key/gift-Pokemon lists with deduplication, locations and owned marks | Android's exact panel layout and further list ergonomics |
| States/options | Existing host menu/state/FF controls | Multi-slot screenshot browser, broader options and remaining lifecycle integration |

Guide definitions are generated from `GuidePolishedCrystal*.kt`; the original
Android guide marks itself unverified and may contain source-data mistakes.
Guide progress is read-only: no event, badge, bag or party writes are introduced.
There are no invented memory offsets or device-side downloaded assets.

No DS, achievements, cloud or new ROM-hack support was added. GBA gameplay and
retail companion support remain as before; this does not add Seaglass decoding.
