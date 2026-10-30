#!/bin/bash
export TMPDIR="${TMPDIR:-/tmp}"
TMPDIR="${TMPDIR%/}"
# ISSUE #42: "Settings are not getting saved (in player caption settings
# mainly)". Roda o mesmo binario em DOIS PROCESSOS — "mata o processo e
# reabre" so prova alguma coisa quando os `static` de player.c voltam ao
# valor de fabrica entre uma chamada e outra, o que um so processo no mesmo
# `main` nao demonstra.
set -eu
cd "$(dirname "$0")/.."

DADOS="$(mktemp -d)"
ARTE="$(mktemp -d)"
trap 'rm -rf "$DADOS" "$ARTE"' EXIT

sources=()
for source in src/*.c src/dts/*.c; do
  if [ "$source" != src/main.c ]; then sources+=("$source"); fi
done
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
cc "${sources[@]}" tests/player_prefs.c -Isrc -o ${TMPDIR:-/tmp}/nuvio-player-prefs-tests "${host_flags[@]}" "${host_libs[@]}"

export NUVIO_DADOS="$DADOS"
export NUVIO_ARTE_TESTE="$ARTE"
${TMPDIR:-/tmp}/nuvio-player-prefs-tests escrever
${TMPDIR:-/tmp}/nuvio-player-prefs-tests ler
${TMPDIR:-/tmp}/nuvio-player-prefs-tests migrar
${TMPDIR:-/tmp}/nuvio-player-prefs-tests migrado
