#!/bin/sh
# NextUI Tools/tg5040/PokeDaisy.pak/launch.sh
set -eu
PAK_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$PAK_DIR"
export LD_LIBRARY_PATH="$PAK_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export POCKET_FULLSCREEN=1
mkdir -p roms saves states
exec ./pokedaisy-pocket --data "$PAK_DIR" "$@" >> "$PAK_DIR/launch.log" 2>&1
