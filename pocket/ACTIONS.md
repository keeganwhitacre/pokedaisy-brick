# Controller-only actions: Phase 15 boundary

The first action is **Start on the companion Bag -> open the original game Bag**.
It does not use the selected companion item. Normal game controls take over
inside the real Bag. Healing/targeting, held items and battle commands are not
implemented by this dispatcher yet. This is a downstream extension, not an
existing Android Polished field-item action.

`game_action.c/.h` accepts a read-only memory callback and returns mGBA controller
bits. It has no mGBA, SDL, filesystem or write-memory API. The host owns input
exclusivity, single-frame pacing even during FF, cancellation and feedback.
Do not add game addresses to the SDL frontend. An eventual upstream port can
reuse the dispatcher contract and replace each game's observations separately.

## Verified source

Game source: https://github.com/Rangi42/polishedcrystal/tree/v3.2.3
(commit `3fa43192379df5c3e7b09a08e4d5d79af4f02f42`).
Release symbols: `polishedcrystal.sym` from the v3.2.3 release, SHA-256
`be1ca87ad487ade20701ed71fcae4e99bf16b908af727b84575c862b73ea2955`.
Only normal 3.2.3 SHA-1 `6930b48af5844d373e3c9130f26d6dd1084cf4ed`
is enabled. Other supported companion ROM builds retain all previous features,
but this action rejects them. No guessed cross-version offsets are used.

| Observation | Official symbol / source | Purpose |
| --- | --- | --- |
| `01:d0f5`, `01:d436`, `01:d431`, `01:d233` | wStateFlags, wScriptMode, wMapStatus, wBattleMode | Require free overworld before starting; reject battles |
| `00:ce61`, `00:ce84` | wMenuDataPointer, wMenuCursorY | Identify Start menu and live cursor |
| `01:d03e` | wMenuItemsList | Read actual available menu IDs, count and terminator |
| `04:6160` | StartMenu.MenuData, engine/menus/start_menu.asm | Recognize the original Start menu |
| `04:4791`, `47a9`, `47c1`, `47d9`, `47f1` | PocketMenuDataHeader.MenuData2, engine/items/pack.asm | Confirm opened Items/Medicine/Balls/Berries/Key pocket |

WRAM1 uses the existing fixed-bank reader (`0x1dxxx`), never the currently
switched CPU bank. These addresses are read, never written. The live menu list
is checked for valid unique IDs, cursor bounds and its terminator; Bag is ID 2,
not a hard-coded row. Six observations establish each waiting/complete state.
The dispatcher presses Start, releases, walks the real cursor and presses A.
The actual game opens its Bag and enforces its normal rules.

## Failure and cancellation

B, X, Menu or L3 cancels, releases synthetic keys and restores control. The
360-frame deadline also releases keys; failed or timed-out dispatch returns
to the companion with a reason. It never guesses a series of B presses to
clean up an unknown screen. Return to gameplay to handle that screen manually.
A remembered TM/HM pocket is not yet a recognized completion state: it may
open correctly but time out. Do not interpret that as successful verification.
Menu/script context is narrower than every possible overworld condition; more
maps, contest states and transitions still require testing before extending it.

Before adding healing: verify Medicine navigation, item identity/quantity,
Use submenu, target party identity, legal injured/non-egg target, waiting cursor
and game-owned HP/quantity completion. Observe dialogue/animation/failure paths.
Never replace that with a RAM HP increment or inventory decrement. The current
proof uses a genuine starter state, advances dialogue with normal buttons and
opens the empty Bag; it proves neither item use nor item targeting.

Phase 17 ports Android's Polished battle command and switch support from
`Gen2Reader.readGen2BattleInput`, `GbPartySwitch.Gen2PartySwitch` and
`BattleInputController` into a separate read-only/controller-bit battle module.
Visible cursor-tile checks and item-target exclusion remain essential.
See INTERACTIONS.md for genuine-ROM versus capture/model verification limits.
