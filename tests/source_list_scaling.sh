#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-source-scaling.XXXXXX")
trap 'rm -rf "$work"' EXIT
sources=()
for source in src/*.c src/dts/*.c; do
  [[ "$source" == src/main.c || "$source" == src/streams.c ]] || sources+=("$source")
done
flags=(-Isrc -O1 -g -DNV_STREAMS_LG=1 -ffunction-sections -fdata-sections -Wno-deprecated-declarations)
if [[ $(uname) == Linux ]]; then
  flags+=(-DNV_LINUX_DESKTOP -Wl,--gc-sections)
  read -r -a cflags <<< "$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf glesv2 egl zlib)"
  read -r -a libs <<< "$(pkg-config --libs sdl2 SDL2_image SDL2_ttf glesv2 egl zlib)"
  libs+=(-ldl -pthread -lm)
else
  cflags=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2)
  libs=(-L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -Wl,-dead_strip)
fi
if [[ ${SANITIZE:-0} == 1 ]]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all); fi
cc "${sources[@]}" tests/source_list_scaling.c "${flags[@]}" "${cflags[@]}" -o "$work/test" "${libs[@]}"
"$work/test"
