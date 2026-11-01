# DS view expansion investigation

The NextUI build still has expansion OFF (no expansion option). This directory
contains a reproducible private experiment, not another supported emulator mode.
No game addresses, ROM patches, memory writes or private game data are added.

## What was inspected

- App upstream main `52f959770de4085bacfda16e6d77be7e91134bc4`:
  `app/src/main/cpp/nds/pk_nds_wide.cpp`. Its software mode still uses eight
  auxiliary slices beside the original view, with overlap/padding and workers.
  Its GPU mode already has a different strategy: one expanded polygon list,
  clipped/reprojected into a wider render target. That is useful prior work to
  reuse if a compatible accelerated backend becomes practical here.
- DSperate pinned `e04b806b5c01527b08a8e7181476ada57e97db52`, plus upstream
  main `f608b4c54e60d2d9f022a7c7a6092c415ea49597`. `gpu3d.cpp/.h` are unchanged
  between these revisions. The software path clips submissions to native
  256x192 geometry and raster buffers. There is no host projection/expanded
  compositor API to call. Our shipped target disables the Vulkan backend.
  A wider target would require changes to clipping, raster buffers, 2D
  composition and capture semantics, not simply SDL destination scaling.
- https://github.com/pret/pokeplatinum/blob/main/src/camera.c : the game's
  camera has perspective and orthographic projection and FOV/distance controls.
  This establishes a camera-level alternative, but there is no app-owned
  portable camera adapter/verified runtime location to use. None was guessed.
- https://github.com/pruefsumme/widemelon : a useful reference for expanded 3D
  with native, centred 2D. Its expanded profiles use the classic OpenGL
  renderer. This is not a drop-in DSperate/Brick dependency.

Sources are GPL-compatible project references; no code from the decompilation
or WideMelon is bundled. DSperate and existing notices remain GPL-3.0. The probe
only generates a small local derivative of its pinned geometry source; its
copyright and license header are retained. There is no new dependency.

## Why a widescreen stretch is insufficient on the Brick

The DS screen and Brick panel are both 4:3 (256x192 and 1024x768). Normal output
already fills the panel at exact 4x scale. Changing the SDL destination cannot
show geometry that was clipped away. A 16:9 letterbox would also use less of
this panel. The useful cheap alternative here is a modest 4:3 zoom-out.

## Single-pass projection proof

`projection_probe.py BUILD OUTPUT` copies just `gpu3d.cpp`, inserts an x/y
clip-coordinate scale before clipping (also recomputing NEON outcodes), compiles
that object with the existing build's flags, and substitutes it into a COPY of
the core archive. It relinks to OUTPUT/pokedaisy-projection-probe. The normal
core, submodule, executable and NextUI package are not modified. Makefile builds
only. Run only with copies of saves/states in a disposable data directory.

```
python3 linux/dsperate/research/projection_probe.py build/native-ds /tmp/ds-probe
PD_DS_PROBE_PERCENT=80 /tmp/ds-probe/pokedaisy-projection-probe --data /tmp/private-test-copy /path/to/your.nds
```

100 is identity; 90 and 80 reduce the submitted scene's size. Values outside
50..100 fall back to 100. The target stays 256x192; emulation and rasterisation
still run once per native frame. It scales the submitted world, not a screenshot:
geometry formerly outside the viewport can reach it. At 80%, its nominal visible
world span is 1/0.8 = 1.25 times native in both axes. Characters and world details
become smaller; there is no extra pixel resolution.

Actual private-cartridge tests used the exact identified Platinum USA rev1 and
a disposable copy of the supplied save/resume. Captures show more Jubilife
scenery at 90/80%, without the auxiliary-pass seams. The menu and dialogue panels in the tested
scene retain their native 2D proportions. Native and AArch64/NEON field captures at all three scales are
pixel-identical, compared separately from the normal build. This is a static-scene proof,
not a playthrough or a performance/compatibility claim.

## Gates before a user-facing option

The probe deliberately scales ALL submitted 3D. It is not safe to turn into a
persisted preference as-is. A supported option needs a small upstreamable core
presentation hook, exact-cart restriction and free-field gating through the
existing field-state interface, with native output restored for battle/menu/
text/warp/cutscene phases, bottom screen and unsupported games. Stale geometry
must be invalidated correctly when the mode changes. Save/load/resume must not
serialize or retain a presentation transform as game data.

Check outdoor/indoor/edge maps, movement, NPC/sprite placement, game-side culling,
weather, screen capture effects, dialogue, menus, transitions and battles. A
projection change cannot recreate objects the game did not submit. Some 2D
weather or effects may not line up with the scaled 3D. Then measure sustained
speed, audio and power on the physical Brick. None is established by screenshots
or QEMU timing. Full expanded views at the original sprite size remain a larger
renderer/compositor task; the app's single-list GPU work should inform it.
