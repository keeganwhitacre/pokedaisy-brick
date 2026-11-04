#!/bin/sh
# An independent pak tag keeps SRAM, states and settings separate from NDS/MGBA.
CORES_PATH=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
EMU_TAG=PDDS
ROM=$1
[ -n "$ROM" ] && [ -f "$ROM" ] || exit 1
cd "$USERDATA_PATH" || exit 1
exec minarch.elf "$CORES_PATH/pokedaisy_ds_libretro.so" "$ROM" > "$LOGS_PATH/$EMU_TAG.txt" 2>&1
