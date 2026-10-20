#!/usr/bin/env bash
# Both production Rust archives must coexist in the same C executable.
set -euo pipefail
cd "$(dirname "$0")/.."
task_tmp=$(mktemp -d)
trap 'rm -rf "$task_tmp"' EXIT
export RUSTUP_HOME="${RUSTUP_HOME:-$HOME/.rustup}"
export CARGO_HOME="${CARGO_HOME:-$HOME/.cargo}"
mkv_lib=$(NUVIO_RUST_EXTERNAL_PERSONALITY=1 bash tools/rust-mkvcore.sh)
NUVIO_DV7_ROOT="$PWD/build/dv7-host" bash tools/build-dv7.sh
cat > "$task_tmp/main.c" <<'C'
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
extern int nv_mkvcore_contentor(const uint8_t *, long);
extern ptrdiff_t nv_dv7_rpu(const uint8_t *, size_t, uint8_t *, size_t);
int main(void) {
  assert(nv_mkvcore_contentor(NULL,0)==0);
  assert(nv_dv7_rpu(NULL,0,NULL,0)==-1);
  return 0;
}
C
cc "$task_tmp/main.c" "$mkv_lib" build/dv7-host/libnuvio_dv7.a \
  -ldl -pthread -lm -o "$task_tmp/test"
"$task_tmp/test"
echo "PASS: official Rust MKV core and DV7 wrapper link and run together"
