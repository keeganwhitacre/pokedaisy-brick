# Save previews and smooth startup

Menu > Save state / Load state opens the ten-slot browser. D-pad selects a
slot, A saves/loads, Y switches Save/Load mode, and B returns to gameplay.
An empty slot saves immediately. Overwrites and loads require a second A;
B cancels confirmation. Changing slots requires a fresh confirmation.
Successful operations return directly to play with released inputs/audio
queues and reset pacing. The quick ring and existing L2/R2 actions keep using
slot 0, preserving the tested controls and old quick-state filename.

Screenshots show the actual game image, not the companion/menu. Stored raw
pixels are converted to RGB for display. Dates are the device's local file
timestamp. Four cards fit at once in two columns; the remaining slots scroll
with the selection. This adapts Android StatesScreen/SlotCard's OPTION windows,
numbered slots and screenshot/timestamp semantics to the Brick's controls.
Android's minimum three columns and touch buttons are adapted; Undo Save and
Undo Load are not included in this increment.

## Storage and compatibility

Slot 0 remains states/<complete ROM basename>.ss. Slots 1–9 append .slot<N>
to that path. Existing battery saves, backups and recovery paths are unchanged.
Old Pocket states remain loadable and display "Old state" without a preview.
Do not rename your ROM if you want existing basename-based files to match.

New slots use mGBA's own extended format. The screenshot, time and ROM/hash
metadata travel in the same file as the state. A temporary file, fsync,
atomic rename and directory fsync avoid a mismatched state/thumbnail pair.
Before loading, new slots check ROM identity and state/preview hashes. File
and extension-record sizes are bounded before invoking mGBA's parser. Failed
loads restore the pre-load core state. Files containing embedded battery
savedata are rejected: manual slots do not replace newer player SRAM.
Legacy files lack the new integrity metadata and rely on core validation.

Automatic recovery remains separate, validated against the current battery
save. Manual slots never become an automatic-resume fallback merely because
they are newer. A graceful close writes recovery when AUTO RESUME is ON;
sudden power loss/forced termination cannot trigger a final save.

## Launch and close

- NextUI game-list entry: the existing optional Emus launcher opens the
  selected ROM directly, sharing the Tools pak's saves and preferences.
- Tools entry with exactly one ROM: starts that ROM directly. Multiple ROMs
  still get the picker. Pass --library to explicitly retain the picker.
- AUTO RESUME ON: Menu > Close game saves recovery and exits without an
  extra confirmation. A single-ROM Tools launch returns to NextUI, rather
  than reopening a redundant ROM picker.
  If recovery writing fails during ordinary play, A retries and B keeps the
  current game running. Forced termination cannot present that choice.
- AUTO RESUME OFF: closing retains the existing warning/confirmation.
- Hold B at startup or pass --fresh to bypass automatic recovery.
- launch.log rotates at 1 MiB, retaining one previous log.

Keep saving in-game. New physical control/power/audio/RTC behavior still needs
Brick testing; Linux/QEMU tests are not device validation.
