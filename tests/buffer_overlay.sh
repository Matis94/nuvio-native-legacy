#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_tmp=$(mktemp -d)
trap 'rm -rf "$task_tmp"' EXIT
read -r -a cflags <<< "$(pkg-config --cflags sdl2)"
cc -std=gnu11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
  -Isrc "${cflags[@]}" tests/buffer_overlay.c -Wl,--gc-sections -o "$task_tmp/test"
"$task_tmp/test"
