# Linux port audit and Summary increment — 2026-10-25

## Checkout and history

Repository: keeganwhitacre/pokedaisy-brick. Started at feature/emerald-summary,
0881e8f, with five modified tracked files and an untracked polished-fixture.png.
Fetching all fork branches revealed 072f8aa already contained exactly those
five modifications. Compared the complete diffs byte for byte, then aligned
with that commit without overwriting any file. Preserved polished-fixture.png.
New work is isolated on feature/summary-exp-items.

Official upstream main was fetched read-only and resolves to 371d34f (1.3.2),
already the integration base. Existing fork branches preserve the staged
linux-font, linux-snapshots, linux-host-options, linux-gb-profile, linux-port
and reference-preview contributions. The stable Brick baseline is 3e6cb54.
No upstream branch was modified and no upstream PR was opened.

## Verified implementation and limits

| Area | Source / verified state | Boundary |
| --- | --- | --- |
| Linux host | pocket/main.c, SDL2 and pinned mGBA; native and real NextUI ARM builds pass | Not Android or DS emulation |
| Isolation | pocket/ beside 3ds/; shared C reused; integration checker passes | Shared prerequisites must land separately before a main-targeted Linux PR builds |
| Polished Crystal | SHA-matched adapter; party/summary, Bag, battle, Dex, Map, Guide; captured-memory decoder tests pass | Several source comments are historical; use current implementation and later phase documents |
| Retail Crystal | mGBA gameplay support | No retail Crystal companion adapter found; do not equate it with Polished |
| Emerald | Real supplied ROM/save: Party, Summary, Stats, Bag, Map, Dex, Settings, expanded world and save-state roundtrip | Screens opening is not exhaustive verification of every possible game state/action |
| FireRed | Retail decoder and shared UI tested against captured RAM | No FireRed ROM/save integration run this session |
| GBA stats/moves | Existing decoded stats, IV/EV, nature, Hidden Power, move names/types/power/PP | Move detail/learn pages and exact Android artwork remain incomplete |
| EXP/items | This increment: total EXP, bar, NEXT/MAX, known NONE, held-item description, adjacent party navigation | Read-only; no TAKE/GIVE or game writes |
| Lifecycle | Recovery, manual states, NextUI event backend, launch wrappers and diagnostic tests exist and pass | Physical suspend, audible audio, RTC and long-session performance need device tests |
| Smart Pro | Existing SDK is derived from TrimUI Smart Pro; platform code is isolated | No verified firmware/input/display/lifecycle profile for Lidor's device yet |

## Tests run in this session

- Shared host/font/stats/preview tests, with C11 -Wall -Wextra -Werror.
- Six Android-derived experience curves across all levels; retail species
  mapping, clamping, invalid inputs, NEXT/MAX and unknown-data concealment.
- All 24 encrypted party orders, checksum rejection, optional stats mechanics,
  captured Emerald/FireRed UI, item/back/stats and party-wrap transitions.
- All 80 compact preview PNGs byte-identical to the saved baseline.
- Complete native Linux and NextUI AArch64 Release frontend/test builds.
- Native and ARM/QEMU summary, host I/O, renderer profile, platform-event,
  favorites, battle model, functional and Polished decoder suites.
- Native and ARM/QEMU diagnostic GB/GBA runtime: companion navigation,
  save/state roundtrips, quick menu and no-mmap fallback. Native launcher tests.
- Actual Emerald save, native and ARM/QEMU: 470 matched original-PPU-equality
  frames, original/expanded/zoom, walking, START fallback, state reload,
  companion screens, EXP, INFO/TYPES, party navigation and MAX capture.
- App-derived generator checks and shared/Linux isolation check.

The real save's three party members hold no item. Held-item detail images are
captured-RAM fixture renders and named accordingly. No game memory injection
was used in the live Emerald integration tests. No physical handheld, Android
runtime or 3DS hardware tests were performed, and local tests do not imply
remote GitHub Actions results.

## Review boundaries and remaining work

1. Initial Linux port + CI: retain the existing upstream/linux-port extraction,
   dependent on separately reviewed fonts/snapshots/host-options/GB-profile
   changes. Do not submit this entire feature branch as the initial port PR.
2. Shared feature: the new 3ds/core experience/summary/decoder changes,
   generator and tests are a portable Summary information contribution. It
   builds on the earlier large-canvas Summary/STATS commits, not pristine main.
3. Linux follow-up: consolidate Polished's EXP wrapper onto that shared API
   and extend the private Emerald runtime harness/documentation. Separate
   from the shared feature when preparing actual upstream PRs.
4. Defer: game-data ownership/refactor, remaining action/battle constants and
   Polished expansion-symbol data, pending Lidor's direction. No duplicate
   memory-offset tables were added here.

Best next step: test this build on the Brick, then agree the existing shared
prerequisite sequence with Lidor and prepare the Linux port comparison against
those reviewed dependencies. Next UI feature: read-only GBA move details and
type badges using app-owned data/assets, with Polished regression coverage.
DS is deliberately deferred; no DS backend or speculative addresses were added.
