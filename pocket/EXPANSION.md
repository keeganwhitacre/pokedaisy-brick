# Phase 18: expanded-world objects

This increment adapts the retained Android wide renderer for normal Polished
Crystal 3.2.3. Map matching, terrain, original PPU pixels, loaded-object
animation and safe menu/battle fallbacks remain. No game RAM, VRAM, palettes,
flags or saves are modified.

## Source

Game source: Rangi42/polishedcrystal v3.2.3,
3fa43192379df5c3e7b09a08e4d5d79af4f02f42. Official normal-release symbols have
SHA-256 be1ca87ad487ade20701ed71fcae4e99bf16b908af727b84575c862b73ea2955.
gen_polished_wide.py verifies that hash and generates the added wMapObjects
and wLoadedObjPal0 addresses. No memory address was inferred from a screenshot.

The previous Polished branch stopped after its 13 loaded objects and zeroed
map objects. polished_wide_objects.inc adapts these actual implementations:

- macros/ram.asm and map_object_constants.asm: 21 map objects of 14 bytes,
  association, visibility mask, coordinates and palette.
- home/map_objects.asm CheckObjectTime: four time bits, including evening.
  map_setup.asm CheckObjectFlag: FFFF always, FFxx hidden; valid flags are
  bounded to the actual NUM_EVENTS.
- attributes.asm, macros/scripts/maps.asm and home/map.asm: combined
  script/event header, counted scene/callback/warp/coordinate/background lists
  sized 2/3/5/5/5, then 13-byte events.
- overworld.asm GetSprite/GetUsedSprite and data/sprites: actual compressed
  standing graphics, using the existing portable LZ decoder. A bounded cache
  is reset on ROM configuration, avoiding per-frame decompression.
- player_object.asm and dynamic_pals.asm: six-bit logical palette id,
  whole-byte override minus one, resolved to a currently applied palette slot.

Loaded actors take precedence over templates. Initial coordinates identify
existing actors, including connection reassociation. Already drawn objects
also suppress overlapping templates. Unspawned actors use actual RAM or
connected-map ROM coordinates and the original standing direction, following
upstream's approach without inventing movement. Small guarded hooks in the
common renderer call the Linux/Polished adapter; other games still read their
exact original object layouts.

## Limits

This is partial coverage, not all-map correctness. Dynamic Pokemon/forms,
decorations and special large/action-specific objects remain omitted. Only
ordinary standing/walking/spinning NPC movement and standing items are
supported. A missing applied palette causes omission rather than wrong color.
Connected-map callbacks are not executed: callback-moved/conditional actors
need more verification. Unloaded wanderers stand at their source position;
loaded actors retain their actual positions/frames. Different-tileset
connections retain the existing safe terrain restriction. Physical Brick
speed, weather/darkness breadth and longer exploration remain pending.

## Tests

pocket-wide-objects-test compares decoded player graphics against original
game VRAM; tests mask/time/association/palette/special-sprite/cache guards;
and parses Route29's actual event header. With POCKET_TEST_OUTDOOR=1, normal
controller input exits Elm's lab, walks through New Bark Town and crosses
west into Route29. Every frame checks the original PPU rectangle byte-for-byte.

The crossing's same-frame templates-on/off comparison shows an additional
connected New Bark NPC at the expanded right edge. Both captures retain the
exact PPU center. Templates-off disables only the new layer in a copied
snapshot; it is not an independently run historical binary.

From pocket, using the genuine aide-Potion state earned by the existing battle
harness with POCKET_TEST_ITEMS_STATE:

```sh
POCKET_TEST_OUTDOOR=1 build-native/pocket-wide-objects-test /private/game.gbc /private/items.ss
build-native/pocket-platform-test
```

Run ARM targets under qemu-aarch64-static with the actual NextUI SDK sysroot.
See VERIFICATION.md and delivered logs for results, not hardware claims.
