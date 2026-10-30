#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# Rascunho, cancelamento e persistencia, sem abrir janela ou fazer requisicoes.
#   bash tests/ajustes_ux_interacao.sh
set -eu
cd "$(dirname "$0")/.."
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/ajustes.c) continue;; esac
  sources+=("$source")
done
binary="$(mktemp ${TMPDIR:-/tmp}/nuvio-ajustes-ux-interacao.XXXXXX)"
trap 'rm -f "$binary"' EXIT
host_flags=(-O1 -g -Isrc -Wno-deprecated-declarations -Wno-macro-redefined)
if [[ $(uname -s) == Darwin ]]; then
  host_flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -L/opt/homebrew/lib)
  host_libs=(-lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL)
else
  read -r -a host_sdl_flags <<< "$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf)"
  read -r -a host_sdl_libs <<< "$(pkg-config --libs sdl2 SDL2_image SDL2_ttf)"
  host_flags+=(-DNV_LINUX_DESKTOP "${host_sdl_flags[@]}")
  host_libs=("${host_sdl_libs[@]}" -lz -lGLESv2 -pthread -lm -ldl)
fi
cc "${sources[@]}" tests/ajustes_ux_interacao.c -Isrc -o "$binary" "${host_flags[@]}" "${host_libs[@]}"
"$binary"
