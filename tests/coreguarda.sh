#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir "$work/app"
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -DNV_WEBOS \
  -Isrc tests/coreguarda.c src/coreguarda.c -o "$work/test"
"$work/test" "$work/app" > "$work/out"
grep -E '^\[coreguard\] core files disabled \(RLIMIT_CORE was soft=[^ ]+ hard=[^ ]+\); removed 1 old crash dump file\(s\), 0\.0 MB$' "$work/out"
grep -Fx '[coreguard] test: data written before _exit' "$work/out"
