#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cc -std=c11 -Wall -Wextra -Werror -DNV_LINUX_DESKTOP -Isrc \
  $(pkg-config --cflags sdl2 glesv2) src/ramoverlay.c tests/playback_ram.c -o "$tmp/ram"
"$tmp/ram"
