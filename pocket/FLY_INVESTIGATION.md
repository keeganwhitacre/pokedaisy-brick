# Polished Crystal companion Fly investigation

Status: source investigation only; no Fly action is enabled by this update.
Android's FieldItems/Fly implementation does not supply a Polished adapter.
This would be a downstream game-specific extension using the existing input
dispatcher, not an already-supported Android Polished feature.

## Verified game behavior

Examined Rangi42/polishedcrystal tag v3.2.3, commit
`3fa43192379df5c3e7b09a08e4d5d79af4f02f42`:

- `engine/events/overworld.asm`, `CheckPartyMove`: accepts a non-egg party
  member that knows Fly, or a compatible member when the player carries HM02.
  Requiring that the move already be taught would incorrectly restrict the game.
- `TownMap_InitFlyPossible` in `engine/pokegear/pokegear.asm` checks that party
  rule, Storm Badge and `CheckFlyAllowedOnMap`.
- `CheckFlyAllowedOnMap` checks the map environment and the explicit indoor
  exception table. Orange access also requires the Shamouti visited spawn.
- `AnimateTownMapFly` uses `data/maps/flypoints.asm` and `HasVisitedSpawn` to
  identify a visited destination. Landmark and spawn IDs are different domains;
  the companion cursor's landmark cannot be used directly as a warp target.
- `PokegearMap_ContinueMap` handles A only when `wTownMapCanFlyHere` permits it,
  then queues the game's `FlyFunction.FlyScript`. The game performs weather,
  animation and transition handling itself.

## Smallest safe implementation path

1. On a supported ROM, validate free field context and remember the requested
   companion landmark. Reuse the existing action-dispatch timeout/cancel model.
2. Open the game's Pokégear Map with controller inputs, observing each actual
   menu transition. Generate any needed observation symbols from this exact
   source/symbol build; no retail Crystal addresses or GBA routines apply.
3. Navigate the game's own cursor to that landmark. Observe the game's
   destination availability, then confirm once with A. Do not write spawn IDs,
   party slots, badges, visited flags, SRAM or warp coordinates.
4. Observe the game completing Fly and returning to free field. On blocked
   context, unexpected dialogue, timeout or cancellation, release every injected
   key and return control with an accurate message.

Opening the game's map is a useful first stage, but must not be described as a
completed companion Fly action. A post-Storm Badge normal-input test state is
needed to verify a genuine flight, region/destination restrictions, HM-compatible
party behavior, cancellation, and exit/relaunch compatibility before shipping
that action. The available early-game test route cannot validate those cases.

The three-region companion map and Places browser remain fully usable and
read-only. No speculative action or false successful flight is exposed.
