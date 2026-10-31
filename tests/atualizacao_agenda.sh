#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/atualizacao.c) continue;; esac
  sources+=("$source")
done
binary="$(mktemp "${TMPDIR:-/tmp}/nuvio-atualizacao-agenda.XXXXXX")"
trap 'rm -f "$binary"' EXIT
flags=(-std=gnu11 -O1 -g -Isrc)
if [[ $(uname -s) == Darwin ]]; then
  flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib)
  libs=(-lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL)
else
  read -r -a sdk_flags <<< "$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf)"
  read -r -a sdk_libs <<< "$(pkg-config --libs sdl2 SDL2_image SDL2_ttf)"
  flags+=(-DNV_LINUX_DESKTOP "${sdk_flags[@]}")
  libs=("${sdk_libs[@]}" -lz -lGLESv2 -pthread -lm -ldl)
fi
if [[ ${SANITIZE:-0} == 1 ]]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${sources[@]}" tests/atualizacao_agenda.c "${flags[@]}" -o "$binary" \
  "${libs[@]}" -Wno-deprecated-declarations -Wno-macro-redefined
"$binary" "$@"
