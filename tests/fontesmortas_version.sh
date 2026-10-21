#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-failure-version.XXXXXX")
trap 'rm -rf "$work"' EXIT
flags=(-O1 -g -Isrc -pthread)
if [[ $(uname -s) == Darwin ]]; then flags+=(-I/opt/homebrew/include); fi
if [[ ${SANITIZE:-0} == 1 ]]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all); fi
cc "${flags[@]}" tests/fontesmortas_version.c src/fontesmortas.c -o "$work/test"
"$work/test"
