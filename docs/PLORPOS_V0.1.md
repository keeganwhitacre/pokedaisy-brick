# PokeDaisy for plorpOS, v0.1

The Linux host of PokeDaisy (mGBA + the companion, `linux/`) as plorpOS's "Play With PokeDaisy" player, with the
companion brought toward the Android app. aarch64, built for the TrimUI Brick; plorpOS downloads this asset.

## Approach

- **One C core, Android's data.** The companion (`3ds/core`) reads everything from the game's ROM and save RAM. Per-hack
  tables are generated from the app's own Kotlin sources (`3ds/tools/gen_*.py`, `gamedata/`), not retyped, so the two
  can't drift. A hack row works only for the exact ROM builds listed (SHA-1); any other build shows the tab as
  unavailable rather than guessing.
- **Verify against Android's recorded output.** DEX against the app's seen/caught sets per fixture save, CARD pixel for
  pixel against the app's recorded CRCs, GUIDE teams sanity-checked from the ROM. Nothing is checked by eye alone where a
  reference exists.
- **Skip what has no data.** No reference, no ROM, or a Kotlin shape the generator can't read: the tab stays off and the
  gap is listed below.

## What is in v0.1

| Area | State |
| --- | --- |
| GBA retail (FireRed, LeafGreen, Emerald, Ruby, Sapphire) | Party, bag, map, guide, dex, card, battle helper, HUD. Unchanged from before; 80 retail preview images identical. |
| GBA hacks, DEX | 18 games' tables verified, incl. Quetzal (0.8.4, 9.x), Recharged Yellow (4 builds). |
| GBA hacks, CARD | Unbound, Lazarus, Recharged Yellow (1.8.0, 1.9.x), SoulGold, Glazed. |
| GBA hacks, GUIDE | Rocket Edition, Ash Gray, GS Chronicles, Radical Red, Orange Islands, Odyssey, Sword and Shield Ultimate Plus, Unbound. Bosses with several teams (difficulties) show one section per team. |
| Shiny marks | SoulGold, R.O.W.E., Elite Redux party mons show a gold `*` (party slot and summary). |
| Game Boy / Color | Black-on-white windows (party, bag, boxes) instead of FireRed's look. Party and bag read from real saves. |
| ROM identity | By SHA-1; PureGreen 2.7.7 row (unverified). |

## Tested

- Host tests (`make -C 3ds test-stats test-dex test-card test-guide`, `scripts/gen_gamedata.py --check`): all pass,
  26 cards, 18 dex games, 8 guide games / 99 teams, shiny decode (SoulGold value + modifier, R.O.W.E., Elite Redux, retail
  never).
- Brick (TrimUI Brick, this build): Blue + Crystal saves (copies, never written back) show party and bag in the new GB
  look; Unbound 2.1.1.1 (b4776b82) loads its save and the GUIDE > NEXT BOSS shows the four difficulty teams.
- Glazed's card matches a screenshot of the game's own card (0 pixels differ).

## Not tested

- Everything on a device other than the Brick; the 3DS build (not compiled here; code size grew, see below).
- On-device: any hack's CARD, DEX or GUIDE besides Unbound's GUIDE; the shiny star on screen (decode is unit-tested,
  drawing is not seen on a real shiny); GB DEX/GUIDE tabs (not built).
- Odyssey and Sword and Shield guides were checked from their ROMs on the host only.
- The Windows/macOS Java app is untouched.

## Parity with the Android app

| | Android | This build |
| --- | --- | --- |
| Gen 3 retail tabs | all | all |
| Hack DEX | all supported hacks | the 18 verified; Celia's probe doesn't match (untested whether Android's check passes) |
| Hack CARD | per hack | 5 hacks (above); Imperium off (no reference render) |
| Hack GUIDE | all | 8 hacks. Not: Amethyst, Gaia, Ghost Grey, Celia, SoulGold, Imperium, Quetzal, Recharged Emerald/Yellow (bosses built by Kotlin helpers/lambdas) |
| Hack MAP | town maps | none (no reference) |
| Shiny icons | recoloured icons + marks | `*` mark only; icons keep normal colours |
| Game Boy look | the game's own ROM font and icons | black-on-white windows with the standard font; no ROM font, no ROM party icons (Pokéball) |
| Game Boy DEX / GUIDE / MAP / CARD | yes | not available ("No guide"/"No Pokédex yet") |
| Battle input / party switch for GB; GB quick-menu items; hack item descriptions | yes | no |
| European Yellow builds | yes | no (no ROMs here) |
| DS | yes | out of scope on Linux |

## Known limitations

- Unbound's VANILLA-difficulty teams list no moves: there is no learnset table for the hack here.
- Unbound's guide shows all four difficulties; Android narrows EXPERT by level.
- The Desktop copy of Unbound v2.1.1.1 (sha1 7c5b9d0d) is a different build and is not recognised; b4776b82 is.
- A Game Boy save must be loaded in game (CONTINUE) before the companion has a party.
- Code size: core objects grew from 0.47 MB to ~1.9 MB (generated text tables dominate); gate them before any 3DS upstream.
- Never use in-game save-state tricks on real saves; saves were never written by tests.

## Use

Extract `pokedaisy-plorpos-v0.1-aarch64.tar.gz` over plorpOS's `pokedaisy` folder (the `pokedaisy-linux` binary plus
licences). Companion: X; R1/L1 change tabs; HUD hotkey as set in plorpOS's in-game menu.
