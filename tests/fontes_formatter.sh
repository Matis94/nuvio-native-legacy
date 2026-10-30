#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Rich formatter, UTF-8 layout and bounded work with 2000 sources; no network.
set -euo pipefail
cd "$(dirname "$0")/.."
FF_DIR=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-fontes-formatter.XXXXXX)
trap 'rm -rf "$FF_DIR"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [[ "$source" != src/main.c && "$source" != src/streams.c ]] && sources+=("$source")
done
flags=(-std=gnu11 -O1 -g -Isrc)
if [[ "$(uname -s)" == Darwin ]]; then
  flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib)
  libs=(-lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL)
else
  flags+=(-DNV_LINUX_DESKTOP)
  read -r -a sdk_flags <<< "$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf)"
  read -r -a sdk_libs <<< "$(pkg-config --libs sdl2 SDL2_image SDL2_ttf)"
  flags+=("${sdk_flags[@]}")
  libs=("${sdk_libs[@]}" -lz -lGLESv2 -pthread -lm -ldl)
fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${sources[@]}" tests/fontes_formatter.c "${flags[@]}" -o "$FF_DIR/test" \
  "${libs[@]}" -Wno-deprecated-declarations -Wno-macro-redefined
NUVIO_DADOS="$FF_DIR" "$FF_DIR/test" "$@"
