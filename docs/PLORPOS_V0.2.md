# PokeDaisy for plorpOS, v0.2 (pre-release)

The Linux host of PokeDaisy (mGBA + the companion, `linux/`) as plorpOS's "Play With PokeDaisy" player, a second step
toward the Android app's companion. aarch64, built for the TrimUI Brick. v0.1's notes: `docs/PLORPOS_V0.1.md`.

`python3 3ds/tools/parity_audit.py` lists what this build has against the Android app, per hack config and per feature,
straight from the sources; the table below is written from its output.

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
