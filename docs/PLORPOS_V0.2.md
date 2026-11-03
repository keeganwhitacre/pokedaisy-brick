# PokeDaisy for plorpOS, v0.2 (pre-release)

The Linux host of PokeDaisy (mGBA + the companion, `linux/`) as plorpOS's "Play With PokeDaisy" player, a second step
toward the Android app's companion. aarch64, built for the TrimUI Brick; plorpOS downloads this asset. v0.1's notes:
`docs/PLORPOS_V0.1.md`.

The goal for v0.2 was the "Parity with the Android app" table: every cell matching the app wherever the data to do it was
on hand. `python3 3ds/tools/parity_audit.py` lists what this build has against the app, per hack config and per feature,
straight from both sources; the table below is written from its output.

## Approach

As in v0.1: one C core (`3ds/core`), the app's data. Every new table is generated from the app's own Kotlin
(`3ds/tools/gen_guide.py`, `gen_shiny.py`, `gen_gb_art.py`, `scripts/gen_gamedata.py`) and checked against what the app's
own tests assert where they assert something; a feature without a reference was checked on the ROMs and saves on hand.

## Parity with the Android app

| | Android | This build |
| --- | --- | --- |
| Gen 3 retail tabs | all | all (the language ports now name places in their own language) |
| Hack DEX | every hack with a POKéDEX record | the same set (the audit finds none missing). Celia: the app's own check reads the same first entry ("ZEROTH") and fails the same way, so neither shows Celia's DEX on its 1.1.4 ROM |
| Hack CARD | per hack | the same, but Imperium: Unbound, Lazarus, Recharged Yellow (1.8.0, 1.9.x), SoulGold, Glazed, Kaizo (retail Emerald's, as the app gives it). Imperium stays off: no reference render to check its card against |
| Hack GUIDE | every hack with a guide | the same set: all 54 hack configs the app gives one. NEXT BOSS with the app's rules (badge counts, middle gyms, League progress vars, difficulty, the starter, Quetzal's three regions), HERE with each game's wild header shape and time-of-day / night sets, WHERE IS from the area data, TIPS |
| Place names (WHERE IS, HERE, MAP label) | every hack | every hack (its ROM's sMapNames for FireRed 1.0 hacks, the app's tables for the rest) |
| Hack MAP | town maps; every FireRed 1.0 hack's own map read from its ROM (RomRegionMap) | unchanged from v0.1: region art where the ROM's blobs match the fingerprints (Radical Red, Rocket Edition, Recharged Yellow, Ultra Violet, Kaizo, Emerald Legacy, Quetzal, Emerald Rogue, Too Many Types 2); "NO MAP" elsewhere, including the FireRed 1.0 hacks the app reads its own map for (Unbound, Odyssey, Gaia, Amethyst, GS Chronicles, Sword and Shield, Orange Islands, Ash Gray). Their place names are read the app's way already. Cursor, labels and player head not checked per hack (no saves) |
| Shiny icons | recoloured icons + marks | the same: the shiny check by ROM hash (every retail cart, 44 hacks), icons recoloured from the shiny palette, gold frame and mark |
| Hack item descriptions | from each ROM | the same (every verified build) |
| Game Boy look | the game's own font and icons | the game's font on every Game Boy game, Red / Blue's Super Game Boy colours, own party icons on Red, Blue, Yellow, Crystal, Gold, Silver. Not yet: Crystal Clear, Prism, Polished Crystal, PureRGB icons (Poké Ball) |
| Game Boy DEX | list with entry pages | Red, Blue, Yellow: list and pages (sprite, category, size, text, stats). Gold, Silver, Crystal, Crystal Clear, Polished, PureRGB: the list only. Prism: none |
| Game Boy GUIDE | HERE, NEXT BOSS, WHERE IS (+ TIPS on Crystal / Polished) | Red, Blue, Yellow: HERE, NEXT BOSS, WHERE IS. Crystal: all four (no headbutt trees in HERE). Not: Polished Crystal, Gold, Silver (the app has none for Gold / Silver either), Crystal Clear, Prism, PureRGB |
| Game Boy MAP / CARD | Pokégear / town map, card | not available |
| GB battle input / party switch; GB quick-menu items | yes | no |
| European Yellow builds | yes | no (no ROMs here) |
| DS | yes | out of scope on Linux |

## Not done (and why)

- **Needs material I don't have**: saves for the MAP-art hacks (Radical Red, Rocket Edition, Recharged Yellow, Ultra Violet,
  Kaizo, Quetzal, Emerald Rogue, Too Many Types 2) to check each map's cursor, labels and head; saves for Yume Blue, Crystal
  Legacy, Ambrosia, Scorched Silver, Delta Emerald and Royal Sapphire (their configs stay guesses, Scorched Silver unmapped);
  a reference render of Imperium's card; the European Yellow ROMs.
- **Left for later**: the FireRed 1.0 hacks' own region maps (RomRegionMap's images and cursor grids), Polished Crystal's GUIDE (badge-scaled levels, rematch rules, faithful teams), Gen 2 DEX pages, Prism's
  DEX, the LZ / sheet party icons (Crystal Clear, Prism, Polished, PureRGB), Crystal's headbutt trees, Game Boy MAP / CARD,
  the Game Boy battle switch and quick-menu items, Elite Redux / R.O.W.E. foe shiny marks in wild battles.

## What changed since v0.1

- **Place names for every hack.** WHERE IS, HERE and the MAP's label name the places in every supported hack, as the app
  does: FireRed 1.0 hacks read their names from the ROM's own region map code, the rest from the app's tables (or their
  engine's retail names); the other-language retail carts in their own language. v0.1 printed "Lying in the GYM" with no
  town.
- **GUIDE for 15 more hacks**, so every hack the app has a guide for has one here: Amethyst (1.3.0, 1.4.1), SoulGold,
  Quetzal (Alpha 0.8.4 and 9.x), Heart and Soul, Lazarus, Seaglass, Too Many Types 2, Glazed, Super Mariomon, Dreamstone
  Mysteries, Ultra Violet, Kaizo (retail Emerald's guide), and HERE + TIPS for Wishes of Tomorrow, Team Rocket Edition and
  Elite Redux. NEXT BOSS follows the app's own rules (badge counts, middle gyms, League progress vars, difficulty, starter,
  Quetzal's regions with the player's first). HERE reads each game's own wild header shape, time-of-day sets and Lazarus's
  night set by the game's clock. WHERE IS for Heart and Soul, Ultra Violet, Amethyst and each Recharged Yellow release's own
  items.
- **Shiny marks and shiny icons as in the app.** The game's shiny check comes by ROM hash, like the app's: every retail
  cart (vanilla's 8) and 44 checked hacks (their own value: Glazed 16, Imperium 36, Odyssey 255, SoulGold 256, ...). A shiny
  party Pokémon, foe or battler gets its icon in its shiny colours (the shiny battle palette mapped onto the icon's, as
  the app does; SoulGold's own shiny icon palettes), a gold frame on the battle cards and the gold `*`. v0.1 marked
  only SoulGold, R.O.W.E. and Elite Redux, with normal icons.
- **Item descriptions from each game's ROM**, as the app's RomItemText: the bag shows a hack's own text (Unbound's,
  Radical Red's, every expansion hack's), read from the item table its config names (checked against item 1's id where
  the records carry one). v0.1 showed none for hacks with their own items.
- **Kaizo's CARD**: retail Emerald's card from Kaizo's ROM, as the app gives it (not checked against a recorded render).
- **Game Boy look closer to the app's**: the game's own font from its ROM (Red, Blue, Yellow, PureRed / PureBlue, Crystal,
  Gold, Silver, Crystal Clear, Prism, Polished Crystal), Red / Blue's Super Game Boy paper and ink (PureRGB's black ink),
  and the game's own party icons for Red, Blue, Yellow, Crystal, Gold and Silver. Still the Poké Ball on Crystal Clear,
  Prism, Polished Crystal and PureRGB (their icons are packed differently) and in the European Yellows.
- **Game Boy DEX pages for Red, Blue and Yellow**: a seen Pokémon's page as the cart has it - the front sprite (Gen 1's
  own compression, in its colours), category, feet / inches and pounds, the text and its five base stats. Crystal, Gold,
  Silver, Crystal Clear, Prism, Polished Crystal and PureRGB keep the list only.
- **Game Boy GUIDE: HERE and WHERE IS for Red, Blue and Yellow**, beside v0.1's NEXT BOSS: the map's wild Pokémon
  (grass / cave, surfing, the three rods, merged like the app's), who gives what and the items lying around (a ball when
  the save has it), under the town map's name, a town's houses counted with the town; WHERE IS built from the same data.
  Game Boy games get their town map names (Yellow's for Gen 1, each Gen 2 game's own landmarks).
- **Crystal GUIDE**: NEXT BOSS for Johto, Kanto and RED (done once the League is won and he's gone from MT. SILVER), HERE
  with the time of day's grass list, surfing, the three rods (the time-of-day fish by the clock) and rock smash, TIPS and
  WHERE IS from its area data. Headbutt trees aren't listed here (the app shows them).

## Tested

Host (x86_64, every commit): `scripts/gen_gamedata.py --check`, `3ds/tools/gen_shiny.py --check`, `gen_gb_art.py --check`,
`make -C 3ds test-stats test-dex test-card test-guide test-shiny test-gbdex test-gbguide test-roms` and the 80 retail
preview images identical to v0.1's (b9e3ab9):

- GUIDE (`test-guide`, every ROM on hand with a guide): 32 ROMs, 596 boss teams read with 1-6 real Pokémon; NEXT BOSS on
  28 saves, 20 of them against the answer the app's own HackGuidesTest asserts (all equal); HERE on the maps the app's tests
  walked (Lazarus's day and night sets, Seaglass, Recharged Yellow's Route 1, Recharged Emerald's Route 114).
- Shiny (`test-shiny`): retail Emerald's wild PIKACHU made shiny (the app's fixture) reads shiny, the lead doesn't, the icon
  is recoloured (looked at: orange, like the game's shiny sprite).
- Place names and item descriptions (`test-roms`): all 58 ROMs with a config name at least 20 places; every verified build
  gives item descriptions from its ROM.
- Game Boy (`test-gbdex`, `test-gbguide`): all 151 DEX pages and sprites of Red, Blue and Yellow; Route 1's HERE in each;
  Crystal's Route 29 by day (50 / 40 / 5 / 5, as pokecrystal's table) and night, RED's two-flag rule, every Crystal team.
- Looked at in preview renders: hack GUIDE pages (Kaizo), Kaizo's card, Blue / Crystal party icons and font, Blue's DEX
  pages, Red's and Crystal's GUIDE pages.

Brick (TrimUI Brick): this build installed and the launcher running (binary md5 = the release tarball's).

## Not tested

- Anything on the Brick beyond installing it: no hands-on pass of the new tabs (the list is in the handoff).
- Hack guides whose ROM on hand is another build than the config's (Amethyst 1.3.0 / 1.4.1, SoulGold, Too Many Types 2
  1.5.2, Super Mariomon 1.4.x): their NEXT BOSS was checked on the app's save fixtures, their teams not read from a ROM.
- Kaizo's card against a recorded render; Gen 1 / Crystal font and icons against the real screens (looked at only).
- The 3DS build (not compiled here). The Windows / macOS Java app is untouched.

## Known limitations

- Unbound's guide shows all four difficulties; the app narrows EXPERT by level.
- Builds guessed onto a neighbour's config ("(unverified)" in their title) may have no item descriptions or guide: their
  tables are checked before use and left off when they don't match.
- Game Boy: a save must be loaded in game (CONTINUE) before the companion has a party, flags or a map.
- Code size grew again (generated tables); gate them before any 3DS upstream.

## Use

Extract `pokedaisy-plorpos-v0.2-aarch64.tar.gz` over plorpOS's `pokedaisy` folder (the `pokedaisy-linux` binary plus
licences). Companion: X; R1/L1 change tabs; HUD hotkey as set in plorpOS's in-game menu.
