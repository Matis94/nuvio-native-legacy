#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
prefix=${NUVIO_DTS_ROOT:-/tmp/nuvio-dts-host}
tmp=$(mktemp -d);trap 'rm -rf "$tmp"' EXIT
extra=()
if [[ ${SANITIZE:-0} == 1 ]];then extra+=(-fsanitize=address,undefined -fno-omit-frame-pointer);fi
cc -std=gnu11 -D_GNU_SOURCE -O1 -g -Wall -Wextra -Werror \
  -DNV_DTS_FFMPEG -DNV_DTS_WEBOS_RECOVERY -DNV_DV_FIXED_AUDIO_TEST \
  "${extra[@]}" -I"$prefix/include" tests/dts_native_policy.c -o "$tmp/test" \
  -L"$prefix/lib" -Wl,--start-group -lavformat -lavcodec -lswresample -lavutil \
  -Wl,--end-group -lm -pthread
ffmpeg -hide_banner -loglevel error -f lavfi -i color=size=64x64:rate=25:duration=1 \
  -f lavfi -i sine=frequency=440:sample_rate=48000:duration=1 \
  -map 0:v -map 1:a -map 1:a -c:v libx264 -bf 0 -g 10 \
  -c:a:0 dca -strict -2 -b:a:0 768k -c:a:1 ac3 "$tmp/source.mkv"
ffmpeg -hide_banner -loglevel error -i "$tmp/source.mkv" -map 0 -c copy -strict -2 "$tmp/source.mp4"
for container in mkv mp4;do
  "$tmp/test" "$tmp/source.$container" "$tmp/original.dts"
  ffmpeg -hide_banner -loglevel error -i "$tmp/source.$container" -map 0:a:0 -c copy -f data "$tmp/reference.dts"
  cmp "$tmp/reference.dts" "$tmp/original.dts"
  rm "$tmp/reference.dts"
done
