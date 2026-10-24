# Phase 22 rendering corrections

The companion's 512x384 logical canvas contains art enlarged two logical pixels
per original art pixel. Sampling the filter as 512x384 texels made it too fine,
and LCD could vanish entirely at a 512x384 renderer output. Its reference grid
is now 256x192. This is tested at both renderer resolutions; physical-display
scaling and appearance still need the user's Brick check.

The battle background was cleared outside lastGameRect, while the effect mask
covered only that rectangle. effect_cover now accepts a destination and a
separate source reference. Its physical mask repeats the same pixel grid into
the margins, preserving source offset and scale. Every filtered centre pixel
must match the old clipped pass, not merely look similar. LCD/SCANLINES,
strengths, color correction and the ON COMPANION opt-out retain their controls.

The original battle wipe/swirl still occupies the PPU viewport. The shared
renderer provides BATTLE_START; when expansion has presented a valid world,
the host retains its last visible RGB image and fades that image behind the
viewport over ten presented game frames. Black transition frames do not erase
that retained scene. The original PPU image is drawn opaquely afterward, so
its flashes, text, timing and pixels are untouched. No terrain is generated
during the transition. Unsupported/non-expanded scenes keep the old fallback.
State loads, new cores and display-mode changes clear retained pixels.

This is a downstream fade of the surroundings, not a full-width recreation
of the game's transition. Retaining the last image adds one bounded buffer
and a copy during expanded rendering; masks remain cached and paused screens
retain Phase 21's redraw optimization. No CPU policy, save format, ROM identity,
gameplay input or emulator architecture changes are introduced.

`pocket-render-parity-test ROM starter-state` uses the existing normal-input
Elm/Lyra route. Captures cover fade stages and both filters on companion and
battle screens at 512 and 1024 wide. The tested framebuffers are software SDL;
actual Brick graphics/audio/power/battery testing is not claimed.
