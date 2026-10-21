#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-audio-flags.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-O1 -g -Isrc -ffunction-sections -fdata-sections)
if [[ $(uname -s) == Darwin ]]; then flags+=(-I/opt/homebrew/include -Wl,-dead_strip); else flags+=(-Wl,--gc-sections); fi
if [[ ${SANITIZE:-0} == 1 ]]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all); fi
cc "${flags[@]}" tests/audio_flags.c src/audio_flags.c src/stream_parse.c src/js.c src/linguas.c src/fontefiltro.c src/badges.c -o "$work/test"
"$work/test"
