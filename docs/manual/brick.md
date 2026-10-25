---
title: TrimUI Brick prototype
summary: Install and use the early native Linux port on a Brick running NextUI.
section: player
order: 99
keywords: [brick, nextui, native, pocket]
---

The native Brick prototype lets you run a supported game and open its companion
with physical buttons. It is an early hardware-test build. Read the included
verification report before using it with your progress.

## Install

Copy the complete `PokeDaisy.pak` folder into `Tools/tg5040` on your SD card.
Put your own uncompressed game files in its `roms` folder, then open the app
from NextUI's Tools menu. Choose a game with the D-pad and A.

## Play and open the companion

Press X to open the companion. The game pauses while you use it. A confirms,
B goes back, and X returns to the game. Settings selections work independently
of the bag's item-list capacity. In the GBA companion, L and R change
sections. In Polished Crystal, L and R cycle Party, Bag, Battle, Dex, Map and Guide. In the bag,
Left and Right change pocket, while Up and Down select items. The bag shows
quantities and money; using an item is not available yet. Summaries include
held items and EXP. A opens a selected item's description. The battle screen is read-only.
In Dex, Up/Down selects, Left/Right pages and A opens an entry; B returns.
Entry text, base stats and sprites come from your matching ROM. Y opens full text and abilities.
The companion now uses the game's own font. Summaries show information on the left and moves on the right; Up/Down changes Pokemon, Left/Right chooses a move and A opens its description. Y cycles defensive types; Select pages long type lists.
On Map, use the D-pad to look around, A to change region, Y to find yourself and Select for Places. Fly and item use remain unavailable.

Press Menu, or hold Select and Start together, for the game menu. You can resume,
open the party, save or load a state, toggle fast-forward, or quit. Loading a
state and quitting ask you to confirm. Save in the game before you quit.

## Game size

The experimental GBA expansion build uses the shared renderer's recognized
games. Expand adds surrounding world for a 4:3 view; Zoom shows a larger area.
Unknown games and unsupported scenes keep the original viewport. These paths
still need real-game verification before replacing a stable build.

The companion keeps its crisp doubled UI. Gameplay uses the physical display
independently: Game Boy and Color use a centered 800 by 720 image, and GBA uses
960 by 640. Both use whole pixels without stretching or cropping.

## Your progress

Your battery saves go in the pak's `saves` folder. The filename is the complete
game filename followed by `.sav`. State files go in `states`. The app makes a
backup before opening an existing save and does not change your other emulator's
files. To import progress, back it up first and copy it into the documented
location. There is no automatic import or conversion.

## What still needs testing

The user has confirmed Phase 1 gameplay, live party and sprites on a physical
Brick. The larger display, bag and Phase 3 battle/dex features still need a hardware retest. The display hotfix
has been tested with a supplied Polished Crystal 3.1.1 game file, but that version
does not support the live companion. Live party data needs a supported 3.2.x
release. Screen captures in the verification folder distinguish the supplied
game from labeled memory fixtures and original diagnostic programs. Read `VERIFICATION.md`
in the package for the exact tested paths and remaining limitations.

## Polished Guide

L/R reaches GUIDE. Left/Right selects HERE: WILD, HERE: FINDS, NEXT BOSS, TIPS or WHERE IS. Y changes wild encounter method or Where Is category. Up/Down chooses an entry; A reveals a hint and then its answer. Up/Down scrolls an open answer, B closes it, and X returns to gameplay. Clock, badges, item/gift and boss marks come from actual memory; no game data is changed. Guide data comes from upstream and may contain mistakes.

## Battle health and map cursor

In the Polished Crystal companion, `INFO`, `SUGGESTIONS`, `FOE DETAILS` and revealed
`FOE TEAM` rows show HP bars in the game's party colors. Green means more than
half, amber means more than one fifth, and red means one fifth or less. Selecting
an unseen foe still reveals its information explicitly; the app does not pretend
the game has announced its next choice.

On `MAP`, the four corners pulse around your selected place. The center stays
clear so you can see your player marker. The game stays paused while you browse
the map or open a summary. Press B to go back or X to resume playing.

## Bag and Dex presentation

The Polished Crystal bag uses the game's magenta stripes and folder tabs.
You start on `All`. Left and Right change pocket, while Up and Down move through
items and show the selected description. A opens or closes the description,
B closes it before leaving, and Select pages longer text. Y cycles `Default`,
`Name`, `Type` and `Count` sorting. This changes your companion view only.
Duplicate stacks stay separate, and sorting keeps your selected stack.
Favorites are not available on Brick yet.

In Dex, the left column shows `SEEN`, `OWNED` and your four filters. The right
column shows each entry's icon, number and name, with caught and seen marks.
Entries you have not encountered are dimmed, like the Android companion.
Up and Down scroll one entry, Left and Right move five, Y changes filter and
A opens the entry. The native entry pages still have their earlier layout.

The shoulder tabs use the Game Boy window style and stay at the top on Brick.
The complete Android settings and state-browser layouts are not available yet.
