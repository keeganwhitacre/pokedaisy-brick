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
