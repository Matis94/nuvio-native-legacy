#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
prefix=${NUVIO_DTS_ROOT:-/tmp/nuvio-dts-host}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cc -std=gnu11 -Wall -Wextra -Werror -Wno-misleading-indentation \
  -DNV_DTS_FFMPEG -Isrc -I"$prefix/include" \
  src/moviebuffer.c src/dts/dts_engine.c src/rede.c src/redeurl.c tests/moviebuffer_daemon.c \
  -o "$tmp/proxy" -L"$prefix/lib" -Wl,--start-group \
  -lavformat -lavcodec -lswresample -lavutil -Wl,--end-group -lm -lpthread -ldl
python3 tests/moviebuffer.py "$tmp/proxy"
