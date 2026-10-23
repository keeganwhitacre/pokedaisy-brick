#!/bin/sh
# NextUI Tools/tg5040/PokeDaisy.pak/launch.sh
set -eu
PAK_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$PAK_DIR"
export LD_LIBRARY_PATH="$PAK_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export POCKET_FULLSCREEN=1
mkdir -p roms saves states
if [ -f launch.log ] && [ "$(wc -c < launch.log)" -gt 1048576 ]; then
  mv -f launch.log launch.previous.log
fi
# NextUI launches standalone paks in Performance. Let its own kernel-governor
# helper scale this app with demand; NextUI restores its policy after exit.
# No helper/older firmware: keep the inherited policy. No sysfs writes here.
cpu_mode=${POCKET_CPU_MODE:-auto}
case "$cpu_mode" in
  auto|powersave|performance)
    if [ -n "${SYSTEM_PATH:-}" ] && [ -f "$SYSTEM_PATH/bin/governor.sh" ]; then
      if sh "$SYSTEM_PATH/bin/governor.sh" "$cpu_mode" >> launch.log 2>&1; then
        printf 'NextUI CPU policy requested: %s\n' "$cpu_mode" >> launch.log
      else
        printf 'NextUI CPU policy request failed; continuing\n' >> launch.log
      fi
    fi
    ;;
  inherit) ;;
  *) printf 'Unknown POCKET_CPU_MODE; keeping inherited policy\n' >> launch.log ;;
esac
exec ./pokedaisy-pocket --data "$PAK_DIR" "$@" >> "$PAK_DIR/launch.log" 2>&1
