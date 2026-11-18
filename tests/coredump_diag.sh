#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-pie -no-pie \
  -Isrc tests/coredump_diag.c src/coredump_diag.c -o "$work/test"
"$work/test"

{
  printf '[coredump]   < [saida] stale copied marker\n'
  for event in $(seq 1 30); do
    printf '[coredump-net] t_ms=%s tid=7013 curl_easy_perform begin\n' "$event"
  done
  printf '[saida] main loop left\n'
  printf '[coredump-net] t_ms=31 tid=7013 curl_easy_perform end rc=56\n'
  printf '[queda] sinal fatal: relato gravado\n'
} > "$work/previous.log"
NUVIO_LOG_ANTERIOR="$work/previous.log" "$work/test" --trace "$work" > "$work/report"

grep -F '[coredump]   < [coredump-net] t_ms=7 tid=7013 curl_easy_perform begin' "$work/report"
grep -F '[coredump]   < [coredump-net] t_ms=31 tid=7013 curl_easy_perform end rc=56' "$work/report"
grep -F '[coredump]   < [queda] sinal fatal: relato gravado' "$work/report"
grep -E '^\[coredump-net\] t_ms=[0-9]+ tid=[0-9]+ curl_easy_perform end rc=56$' "$work/report"
grep -E '^\[saida\] atexit: early-registered marker .* t_ms=[0-9]+ tid=[0-9]+$' "$work/report"
if grep -Eq 'stale copied marker|t_ms=6 tid=7013' "$work/report"; then
  echo 'Unexpected old entry in previous-session trace' >&2
  exit 1
fi
