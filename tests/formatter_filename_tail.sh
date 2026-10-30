#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_dir=$(mktemp -d);trap 'rm -rf "$task_dir"' EXIT
read -r -a flags <<< "$(pkg-config --cflags sdl2 SDL2_image SDL2_ttf)"
cc -std=gnu11 -O1 -g -DNV_LINUX_DESKTOP -ffunction-sections -fdata-sections \
 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc "${flags[@]}" \
 tests/formatter_filename_tail.c src/limpa.c src/dobra.c -Wl,--gc-sections -lm -o "$task_dir/test"
"$task_dir/test"
