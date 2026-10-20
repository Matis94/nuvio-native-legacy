#!/bin/bash
# Real UI renderer, both source-text modes; needs an OpenGL context.
set -euo pipefail
cd "$(dirname "$0")/.."
out=${1:-/tmp/nuvio-fullnames}
mkdir -p "$out"
out=$(cd "$out" && pwd)
dir=$(mktemp -d)
trap 'rm -rf "$dir"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [ "$source" != src/main.c ] && sources+=("$source")
done
flags=(-DNV_SHOT_HOOKS -Isrc -O1 -g)
if [ "$(uname)" = Linux ]; then
  flags+=(-DNV_LINUX_DESKTOP)
  read -r -a cflags <<< "$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf glesv2 egl zlib)"
  read -r -a libs <<< "$(pkg-config --libs sdl2 SDL2_image SDL2_ttf glesv2 egl zlib)"
  libs+=(-ldl -pthread -lm)
else
  cflags=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
  libs=(-L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL)
fi
cc "${flags[@]}" "${cflags[@]}" "${sources[@]}" tests/fontes_nome_completo_shot.c \
  -o "$dir/shot" "${libs[@]}"
NUVIO_DADOS="$dir" NUVIO_TESTE_DIR="$dir" "$dir/shot" "$out/names"
