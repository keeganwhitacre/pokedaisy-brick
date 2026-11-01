#!/bin/sh
# Build the plorpOS release of PokeDaisy's Linux host: one aarch64 binary for
# every plorpOS device. It needs only libSDL2-2.0.so.0 and glibc 2.27, so it is
# linked against the Brick's SDL2 (the oldest firmware) and runs on the others.
#
#   plorpos/build.sh [TAG]   -> plorpos/dist/pokedaisy-plorpos-TAG-aarch64.tar.gz (+ .sha256)
#
# Needs: docker (podman), an image with aarch64-linux-gnu gcc/g++/cmake
# (TOOLCHAIN_IMAGE, plorpOS's tortos-toolchain), and SYSROOT: a directory with
# usr/lib + usr/include for the Brick (plorpOS's mk/fetch-sysroot.sh makes it;
# it holds TrimUI's libraries, so it is not in this repository and this build
# does not run in CI). ~2 minutes.
# SPDX-License-Identifier: GPL-3.0-or-later
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TAG=${1:-dev}
IMAGE=${TOOLCHAIN_IMAGE:-tortos-toolchain}
SYSROOT=${SYSROOT:?set SYSROOT to the Brick sysroot (plorpOS mk/fetch-sysroot.sh)}
OUT=$ROOT/plorpos/dist
command -v docker >/dev/null || { echo "need docker" >&2; exit 1; }
[ -d "$SYSROOT/usr/lib" ] || { echo "no $SYSROOT/usr/lib" >&2; exit 1; }
cd "$ROOT"
git submodule -q update --init --force third_party/mgba
git -C third_party/mgba clean -qfdx
rm -rf build

docker run --rm -v "$ROOT:/w:z" -v "$SYSROOT:/sysroot:ro,z" -w /w $IMAGE sh -c '
set -e
cat > /tmp/tc.cmake <<TC
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_FIND_ROOT_PATH /sysroot)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-a53")
TC
cmake -S linux -B build -DCMAKE_TOOLCHAIN_FILE=/tmp/tc.cmake -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_LTO=OFF -DBUILD_TESTING=OFF >build.cfg.log
cmake --build build -j"$(nproc)" >build.log 2>&1 || { grep -B2 -A8 "error" build.log | head -40; exit 1; }
aarch64-linux-gnu-strip -o build/pokedaisy-linux.stripped build/pokedaisy-linux
# Never ship one a device cannot load: libSDL2 and libc only, nothing past
# the Brick firmware glibc 2.33.
aarch64-linux-gnu-readelf -d build/pokedaisy-linux | grep NEEDED | grep -vqE "libSDL2-2.0|libm|libpthread|libc\." \
  && { echo "!! unexpected library" >&2; exit 1; }
echo "glibc: $(aarch64-linux-gnu-objdump -T build/pokedaisy-linux | grep -oE "GLIBC_2\.[0-9]+" | sort -uV | tail -1)"
' 2>&1 | tail -5

BIN=$ROOT/build/pokedaisy-linux.stripped
[ -f "$BIN" ] || { echo "!! build failed (see $ROOT/build.log)" >&2; exit 1; }
rm -rf "$OUT"
D=$OUT/pokedaisy
mkdir -p "$D/licenses"
cp "$BIN" "$D/pokedaisy-linux"
cp LICENSE NOTICE "$D/"
# The notices linux/package.py ships, the same files.
M=third_party/mgba
cp "$M/LICENSE" "$D/licenses/mGBA-MPL-2.0.txt"
cp "$M/src/third-party/blip_buf/license.txt" "$D/licenses/blip_buf-LGPL-2.1.txt"
cp "$M/src/third-party/inih/LICENSE.txt" "$D/licenses/inih-BSD.txt"
echo "Pixel Operator by Jayvee Enaguas. CC0. See NOTICE." > "$D/licenses/PixelOperator.txt"
# GPL-3.0 section 6: where the exact source is.
cat > "$D/SOURCE.txt" <<SRC
PokeDaisy Linux host for plorpOS, GPL-3.0. Source:
  https://github.com/thwonp/pokedaisy-plorp tag $TAG, commit $(git rev-parse HEAD)
  (a fork of https://github.com/lidor30/pokedaisy)
  with mGBA $(git -C $M rev-parse HEAD)
SRC
A=pokedaisy-plorpos-$TAG-aarch64.tar.gz
tar -C "$OUT" --owner=0 --group=0 --numeric-owner --sort=name --mtime=@0 -czf "$OUT/$A" pokedaisy
(cd "$OUT" && sha256sum "$A" > "$A.sha256")
echo "  built  $OUT/$A"
echo "  binary sha256 $(sha256sum "$D/pokedaisy-linux" | cut -d' ' -f1)"
cat "$OUT/$A.sha256"
