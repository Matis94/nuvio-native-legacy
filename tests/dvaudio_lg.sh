#!/bin/bash
# A troca de TrueHD por E-AC-3 para manter o DV (dvPronto): a faixa ATUAL da TV
# e traduzida para a faixa do MKV por idioma/codec/canais antes de escolher a
# substituta. Compila o ramo LS2 real de src/video.c no host (como
# video_pausa_lg), com o barramento e o motor do DV dublados.
set -euo pipefail
cd "$(dirname "$0")/.."
dir=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-dvaudio-lg.XXXXXX")
trap 'rm -rf "$dir"' EXIT
export NUVIO_DADOS="$dir/dados"
mkdir -p "$NUVIO_DADOS"
flags=(-O0 -g -std=gnu11 -D_GNU_SOURCE -ffunction-sections -fdata-sections -Isrc -Wno-deprecated-declarations)
if [[ $(uname -s) == Darwin ]]; then
  flags+=(-I/opt/homebrew/include -I/opt/homebrew/include/SDL2 -Wno-macro-redefined)
  libs=(-L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf -lz -framework OpenGL -Wl,-dead_strip)
else
  sdl_config=$(pkg-config --cflags --libs sdl2)
  read -r -a sdl_flags <<< "$sdl_config"
  libs=("${sdl_flags[@]}" -lSDL2_image -lSDL2_ttf -lz -lGL -ldl -pthread -lm -Wl,--gc-sections)
fi
if [ "${SANITIZE:-0}" = 1 ]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
sources=()
for source in src/*.c src/dts/*.c; do
  case "$source" in src/main.c|src/video.c) continue;; esac
  sources+=("$source")
done
cc "${flags[@]}" "${sources[@]}" tests/dvaudio_lg.c -o "$dir/test" \
  "${libs[@]}"
"$dir/test"
