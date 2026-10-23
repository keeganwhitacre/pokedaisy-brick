# Phase 16: handheld lifecycle and recovery

Physical sleep/wake, audible audio, startup B and elapsed RTC behavior await
Brick testing. Linux/QEMU tests substitute the suspend callback and never
suspend or shut down their host. This keeps the existing SDL/mGBA backend.

## Expanded conversations

Android `WideCamera.frame` and `Prefs.expandZoomOnText` define AUTO ZOOM IN.
Menu > Settings now exposes it per ROM. Android defaults ON; this build
defaults OFF by user request. Existing Phase 15 filters migrate unchanged.
Only a fully ready MAP frame classified FIELD or (with AUTO ZOOM IN OFF) TEXT
can expand. Menus, battles, unmatched maps and unsafe transitions retain the
original viewport. The shared renderer is unchanged and copies the original
160x144 PPU center exactly, including text, borders and sprites. No stretching
or speculative terrain was added. Exact Android timed camera tweening remains
unported.

Polished's adapter already translates its actual loaded 34-byte object structs.
The normal 3.2.3 source object layout and sprite setup are the audit references
(`constants/map_object_constants.asm`, `engine/overworld/map_objects.asm`).
The Crystal unspawned/connected-map NPC-template path is deliberately skipped
for Polished in `gbObjsOffScreenCrystal`. Objects not loaded by the game can
therefore pop in at expanded edges; this remains an explicit gap. No guessed
object data was added. Laboratory dialogue tests do not certify connected-map
travel, outdoor weather/lighting or every menu transition.

## NextUI power ownership

Reference: LoveRetro/NextUI `5291344b3aceae5fa197437a70824afb41e2d338`:
`workspace/all/common/api.c` (PWR_update, PWR_sleep, PWR_deepSleep),
`workspace/tg5040/platform/platform.h`, `workspace/tg5040/keymon/keymon.c`,
`skeleton/SYSTEM/tg5040/bin/suspend` and
`skeleton/SYSTEM/tg5040/paks/MinUI.pak/launch.sh`.

The foreground app owns power while launched; keymon owns volume/brightness.
The adapter activates only with PLATFORM=tg5040 and an absolute SYSTEM_PATH.
It opens evdev devices advertising KEY_POWER, without grabbing them. If those
are available, SDL power events are suppressed as duplicates; SDL is fallback
otherwise. On press gameplay stops between frames, keys/repeat state clear,
pending game actions cancel and audio pauses/clears. Short release requests
real suspend; a one-second hold requests graceful shutdown. Wake edges are
drained and debounced for one second.

Before suspend the app attempts recovery, closes audio, executes the existing
SYSTEM_PATH/bin/suspend and waits. That helper uses real kernel suspend,
platform hooks, mixer-state capture and Wi-Fi/Bluetooth stop/restart. Its mixer
restore lines are commented out upstream; the app reopens SDL audio itself.
On return queued input is discarded, textures recreated, audio reopened and
frame pacing reset. CPU expansion data stays intact; emulation never catches
up sleeping frames. RTC uses mGBA's existing clock source, without RAM edits.

Missing/failing helpers restore the app and show a menu error; no fake sleep
fallback is used. NextUI's optional backlight-only hybrid delay, monitor-daemon
STOP/CONT, LED/haptics and configuration UI are not reproduced. These firmware
interactions still need hardware evaluation. Long hold exits through recovery
and ordinary save/core/SDL cleanup, then fsyncs `/tmp/poweroff`; the returning
NextUI launcher performs shutdown. The app does not invoke poweroff directly.

## Recovery sessions

AUTO RESUME defaults ON, with a per-ROM toggle in Menu > Settings. A graceful
close or handled SIGINT/SIGTERM saves `recovery/<ROM SHA-1>.session`; sleep also
attempts a save. AUTO RESUME OFF disables loading and writing without deleting
files. Existing battery/manual-state paths and prelaunch backup policy remain.

The separate envelope validates ROM identity, SRAM size and content, state
length (bounded to 16 MiB) and state checksum before core loading. Only actual
SRAM bytes are fingerprinted: GB RTC trailers change at exit. A changed battery
rejects an older recovery regardless of mtime. Serialization uses SAVESTATE_RTC
only, never SAVESTATE_SAVEDATA, so no recovery SRAM replaces the player's
battery save. The current core's own mapped battery writes are synchronized
and the battery file fsynced before creating recovery.

Writes use mkstemp, fflush/fsync, atomic rename and directory fsync. Failures
before rename preserve the prior session. A directory-fsync failure after
rename can mean the replacement is already present; failing storage cannot
provide a durability guarantee. Invalid/truncated/wrong-ROM/changed-battery
sessions fall back to normal boot and log why. Incompatible core loads reset.

Hold B while the app starts, or pass `--fresh`, to bypass loading. The CLI path
is testable without hardware. `--fresh` still permits a replacement session on
graceful close. To retain a suspect file, disable AUTO RESUME or move its
`.session` aside while the app is closed. Always save in-game too: sudden
power loss, SIGKILL, crashes or forced shutdown cannot trigger a final save.

## Direct launch from the NextUI game list

Keep Tools/tg5040/PokeDaisy.pak installed. Copy the optional
Emus/tg5040/POKEDAISY.pak from the archive and place the ROM in a folder tagged
`(POKEDAISY)`, e.g. `Roms/Polished Crystal (POKEDAISY)/`. The selected ROM opens
directly; Close game returns to NextUI. GBC/GBA packages are untouched. Both
routes use the same Tools data root. Retain the ROM's complete basename for
existing battery and manual states to match; neither is automatically imported.

## Reproduction

Build POCKET_TESTS=ON. From pocket, generate original diagnostics with
`python3 make_diagnostic.py`. Create a disposable genuine starter and test:

```
pocket-rom-test /private/game.gbc boot-polished-newgame.txt /private/starter.ss
pocket-session-test /private/game.gbc /private/starter.ss
```

Headless tests use SDL_VIDEODRIVER=offscreen, SDL_RENDER_DRIVER=software,
SDL_AUDIODRIVER=dummy and POCKET_FULLSCREEN=1. ARM runs use the actual NextUI
SDK sysroot through qemu-aarch64-static. See VERIFICATION.md for completed
checks, rather than treating the existence of the harness as a passing result.
