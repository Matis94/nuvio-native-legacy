#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# #479: hidden-window RGB/alpha pixel assertions and zero-rasterization regression; no network.
set -euo pipefail
cd "$(dirname "$0")/.."
FF_DIR=$(mktemp -d ${TMPDIR:-/tmp}/nuvio-fontes-formatter.XXXXXX)
trap 'rm -rf "$FF_DIR"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [[ "$source" != src/main.c && "$source" != src/streams.c ]] && sources+=("$source")
done
flags=(-Isrc -O1 -g -Wno-deprecated-declarations -Wno-macro-redefined)
if [ "$(uname)" = Linux ]; then
  flags+=(-DNV_LINUX_DESKTOP)
  read -r -a cflags <<< "$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf glesv2 egl zlib)"
  read -r -a libs <<< "$(pkg-config --libs sdl2 SDL2_image SDL2_ttf glesv2 egl zlib)"
  libs+=(-ldl -pthread -lm)
else
  cflags=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
  libs=(-L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL)
fi
cc "${sources[@]}" tests/fontes_formatter.c "${flags[@]}" "${cflags[@]}" \
  -o "$FF_DIR/test" "${libs[@]}"
NUVIO_DADOS="$FF_DIR" "$FF_DIR/test" "$@"
