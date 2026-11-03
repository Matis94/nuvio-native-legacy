#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_dir=$(mktemp -d); trap 'rm -rf "$task_dir"' EXIT
flags=(-std=c11 -O1 -g -Wall -Wextra -Werror -Isrc)
if [[ ${SANITIZE:-0} == 1 ]]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
cc "${flags[@]}" tests/sourceperf.c tests/sourceperf_peer.c -o "$task_dir/off"
"$task_dir/off"
cc "${flags[@]}" -DNV_SOURCE_PERF_DIAG tests/sourceperf.c tests/sourceperf_peer.c -o "$task_dir/on"
"$task_dir/on"
"$task_dir/on" --bench
