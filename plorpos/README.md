# plorpOS fork of PokeDaisy

This fork carries the changes that make PokeDaisy's Linux host the "Play With
PokeDaisy" player of [plorpOS](https://github.com/thwonp/plorpOS): plorpOS
settings and hotkeys, the plain Brick's Fn keys, ROM identity by SHA-1, records
for the Gen 3 hacks ported from the Android app's configs, Emerald Legacy,
Ruby, Sapphire and retail gaps, and the HUD by buttons.

- `main` = upstream release + these commits, one per feature or fix. It is
  rebased onto each new upstream release tag; tags `plorpos-vN` are releases.
- `plorpos/build.sh TAG` builds the aarch64 release asset plorpOS downloads
  (needs plorpOS's Brick sysroot, so it runs locally, not in CI).
- Changes are meant to be sent upstream as clean PRs where upstream wants them.
