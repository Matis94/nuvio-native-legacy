#!/bin/sh
# External audio: video of one MKV with the audio of another (engine level).
# Requires the ffmpeg CLI and the host FFmpeg libraries of
# tools/build-dts-ffmpeg.sh (NUVIO_DTS_HOST=1 CC=gcc), like tests/dts_engine.sh.
set -eu
cd "$(dirname "$0")/.."
PREFIX=${NUVIO_DTS_ROOT:-${TMPDIR:-/tmp}/nuvio-dts-host}
TMP=$(mktemp -d "${TMPDIR:-/tmp}/nuvio-ext-audio.XXXXXXXX")
trap 'rm -rf "$TMP"' EXIT
SAN=""
if [ "${SANITIZE:-0}" = 1 ]; then SAN="-fsanitize=address,undefined -fno-omit-frame-pointer"; fi
# shellcheck disable=SC2086
${CC:-gcc} $SAN -std=gnu11 -Wall -Wextra -Werror -DNV_DTS_FFMPEG -DNV_DTS_WEBOS_RECOVERY -I"$PREFIX/include" \
  src/dts/dts_engine.c tests/dts_external_audio.c -o "$TMP/ext" \
  -L"$PREFIX/lib" -Wl,--start-group -lavformat -lavcodec -lswresample -lavutil -Wl,--end-group -lm -lpthread
printf '1\n00:00:00,500 --> 00:00:01,500\nExternal audio fixture\n' > "$TMP/sub.srt"
# Main source: 25 fps video, its own DTS track (never read) and a subtitle.
ffmpeg -hide_banner -loglevel error \
  -f lavfi -i 'color=size=64x64:rate=25:duration=4' \
  -f lavfi -i 'sine=frequency=300:sample_rate=48000:duration=4' -i "$TMP/sub.srt" \
  -map 0:v -map 1:a -map 2:s -c:v libx264 -bf 0 -g 25 -c:a dca -strict -2 -ac 2 -c:s srt \
  -metadata:s:a:0 language=eng "$TMP/main.mkv"
# Another version: its own video, E-AC-3 English and AC-3 5.1 Polish.
ffmpeg -hide_banner -loglevel error \
  -f lavfi -i 'color=c=red:size=64x64:rate=25:duration=4' \
  -f lavfi -i 'sine=frequency=500:sample_rate=48000:duration=4' \
  -f lavfi -i 'aevalsrc=0.1*sin(2*PI*700*t)|0.1*sin(2*PI*700*t)|0.1*sin(2*PI*700*t)|0.1*sin(2*PI*100*t)|0.1*sin(2*PI*700*t)|0.1*sin(2*PI*700*t):s=48000:d=4:c=5.1' \
  -map 0:v -map 1:a -map 2:a -c:v libx264 -bf 0 -g 25 -c:a:0 eac3 -ac:a:0 2 -c:a:1 ac3 \
  -metadata:s:a:0 language=eng -metadata:s:a:1 language=pol "$TMP/other.mkv"
# A version with only DTS: refused for now.
ffmpeg -hide_banner -loglevel error \
  -f lavfi -i 'color=size=64x64:rate=25:duration=4' \
  -f lavfi -i 'sine=frequency=300:sample_rate=48000:duration=4' \
  -map 0:v -map 1:a -c:v libx264 -c:a dca -strict -2 -ac 2 "$TMP/dts.mkv"
"$TMP/ext" "$TMP/main.mkv" "$TMP/other.mkv" "$TMP/dts.mkv"
