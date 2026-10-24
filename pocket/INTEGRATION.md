# Linux upstream integration candidate

This is an integration branch, not a replacement for the installed Brick release.
The source baseline is `stable/brick-phase22` at
`3e6cb54bbd49950217d264f53e2690741ecc4a83`. Its package and save paths remain
unchanged. No save conversion, emulator replacement or UI redesign is part of
this extraction.

## Upstream base and compatibility

Based on upstream `371d34f23de0a1bb10078fdb82aea9041ecb4617` (1.3.2).
The ten upstream commits since our previous base merge without conflicts.
Gen2Reader and Gen2Art provenance hashes changed; regenerating the definitions
changes the manifest only, not the generated C data. Nickname behavior added by
upstream is not automatically implemented by refreshing those hashes.

The `pocket/` name is retained for this first extraction to preserve paths.
The folder is the Linux host beside `3ds/`; it is not a second emulator core.
It builds the existing shared C core and pinned mGBA submodule directly.

## Proposed review sequence

Each item is a separate review boundary. Shared changes must land before the
port can build on main; a stacked comparison is not an independently mergeable
PR against main. Exact grouping/API is subject to maintainer review.

1. **Portable fonts/text**: caller-owned glyph fallback, explicit newlines and
   generated middle-dot glyph. No ROM bytes or host dependencies.
2. **Snapshot capacity/telemetry**: larger pockets and optional live fields.
   Fix the discovered settings/item hit overlap by dispatching item hits only
   on the Bag tab. The optional game-specific progress fields need review with
   the forthcoming data/snapshot refactor.
3. **Host UI options**: optional empty-list guidance and a host capability to
   hide the legacy screen-mode setting. Default 3DS behavior is retained;
   there are no POCKET_NATIVE branches in the shared core.
4. **GB renderer profile interface**: Polished rendering and ordinary object
   handling remain shared, while the host supplies verified addresses and a
   bounded decompressor. No pocket imports, ROM identities, Linux calls or
   Brick controls remain in the shared renderer. Android does not call the new
   profile initializer and does not gain an advertised game-support claim.
5. **Linux folder and CI**: SDL/mGBA host, game adapters, generated definitions,
   NextUI lifecycle/launch wrappers, tests, packaging and documentation.
6. **Reference preview tooling**: the existing real-Gen2 capture additions to
   ui-preview are a separate tooling contribution, not a port dependency.

## Important maintainer decisions still open

- This is a tested extraction candidate, not a declaration that all APIs are
  final or that the complete Android experience has been ported.
- The small renderer profile initializer uses caller-owned, process-wide data,
  like the existing renderer configuration. Agree on its shape before merge.
- Most companion definitions come from Android generators. The existing
  expansion profile still comes from checksum-pinned official game symbols,
  not the app's tables. Coordinate that data ownership with the planned
  upstream refactor; do not silently present it as an app-derived table.
- The existing game action/battle modules contain version-specific observations
  and constants. Audit their ownership against the new upstream data interface
  before claiming the "single data source" goal is complete.
- The historical phase documents are retained for provenance. Their older
  todo lists can be stale; later implemented favorites and preview slots are
  not missing. Presentation and functionality parity remain incomplete.
- The NextUI backend is only tested for the documented Brick environment.
  Smart Pro firmware compatibility and physical lifecycle behavior need device
  testing. No universal handheld or battery-endurance claim is made.

## Verification for this extraction

- Native Linux and actual NextUI AArch64 Release builds, including test targets.
- All five app-derived generator checks.
- Native and ARM/QEMU host I/O, profile lifecycle, power-event model,
  favorites, battle model, functional, decoder and diagnostic runtime tests.
- Normal-input private Crystal starter creation; no game RAM injection.
- Native and ARM/QEMU real-ROM recovery/relaunch/corruption, save previews,
  battle input, object rendering and expanded/filter/fade regression tests.
- Original portable frontend builds with C11 and -Wall -Wextra -Werror.
  All 80 default preview PNGs are byte-identical to a pristine upstream build.
- New host regression checks cover menu defaults/custom guidance and settings
  selection with 128-entry Bag storage. The old branch swallowed settings
  hits 600-604 as item rows 100-104; the new tab guard fixes that collision.
- Inspected actual retained-world, filtered battle/companion and original
  portable settings captures. Test images and private ROM/state files stay out
  of git and install packages.

Remote CI and Android/3DS hardware execution are not implied by local results.
No new physical Brick testing was performed during this integration run.
