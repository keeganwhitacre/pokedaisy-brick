#!/bin/sh
# Optional Emus/tg5040/POKEDAISY.pak: retain the Tools pak and its data root.
set -eu
if [ "$#" -lt 1 ]; then
  echo "PokeDaisy: no ROM selected" >&2
  exit 1
fi
PD_PAK="${SDCARD_PATH:-/mnt/SDCARD}/Tools/tg5040/PokeDaisy.pak"
if [ ! -x "$PD_PAK/launch.sh" ]; then
  echo "PokeDaisy Tools pak is missing: $PD_PAK" >&2
  exit 1
fi
exec "$PD_PAK/launch.sh" "$@"
