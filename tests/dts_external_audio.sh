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
# A 1 s clip with usable audio: what a debrid serves for a file not in cache.
ffmpeg -hide_banner -loglevel error \
  -f lavfi -i 'color=size=64x64:rate=25:duration=1' \
  -f lavfi -i 'sine=frequency=300:sample_rate=48000:duration=1' \
  -map 0:v -map 1:a -c:v libx264 -c:a ac3 "$TMP/short.mkv"
# An MP4 version, Polish AC-3, whose picture is one large keyframe a second
# and tiny frames between: its audio samples gather into about one chunk per
# second, as in a coarsely interleaved WEB-DL, and audio only skips the
# keyframes.
ffmpeg -hide_banner -loglevel error -f lavfi -i 'nullsrc=s=1280x720:d=0.04,geq=random(1)*255:128:128' \
  -frames:v 1 "$TMP/noise.png"
ffmpeg -hide_banner -loglevel error -loop 1 -framerate 25 -t 20 -i "$TMP/noise.png" \
  -f lavfi -i 'sine=frequency=600:sample_rate=48000:duration=20' \
  -map 0:v -map 1:a -c:v libx264 -preset ultrafast -qp 10 -g 25 -bf 0 -pix_fmt yuv420p \
  -c:a ac3 -ac 2 -metadata:s:a:0 language=pol -movflags +faststart "$TMP/other.mp4"
# A Polish dub in AAC at 44.1 kHz, the format of the WEB-DLs in the owner's log.
ffmpeg -hide_banner -loglevel error \
  -f lavfi -i 'color=size=64x64:rate=24000/1001:duration=4' \
  -f lavfi -i 'sine=frequency=400:sample_rate=44100:duration=4' \
  -map 0:v -map 1:a -c:v libx264 -c:a aac -ac 2 -ar 44100 -metadata:s:a:0 language=pol "$TMP/aac44.mkv"
# Long versions for the seek cost: a 60 s source, and a 60 s other version with
# a heavy picture, with its index (Cues) and without one (a stream muxed to a
# pipe has none). Seeking to 50 s must not read the file from the start.
ffmpeg -hide_banner -loglevel error \
  -f lavfi -i 'color=size=64x64:rate=25:duration=60' \
  -f lavfi -i 'sine=frequency=300:sample_rate=48000:duration=60' \
  -map 0:v -map 1:a -c:v libx264 -bf 0 -g 25 -c:a ac3 "$TMP/main60.mkv"
ffmpeg -hide_banner -loglevel error \
  -f lavfi -i 'nullsrc=s=320x180:d=60,geq=random(1)*255:128:128' \
  -f lavfi -i 'sine=frequency=500:sample_rate=48000:duration=60' \
  -map 0:v -map 1:a -c:v libx264 -preset ultrafast -qp 30 -g 25 -bf 0 -pix_fmt yuv420p \
  -c:a ac3 -metadata:s:a:0 language=pol "$TMP/long.mkv"
ffmpeg -hide_banner -loglevel error -i "$TMP/long.mkv" -c copy -f matroska - < /dev/null > "$TMP/long_nocues.mkv"
# "Match automatically": a shared bed of random hits (music and effects) with
# different "speech" on each; the dub's bed is 0.7 s late.
python3 - "$TMP" <<'PY'
import random, struct, sys, wave
R = 48000
def bursts(seed, n, count, amp):
    rnd = random.Random(seed); out = [0.0] * n
    for _ in range(count):
        at = rnd.randrange(n); a = amp * (0.3 + rnd.random()); ln = rnd.randrange(R // 20, R // 4)
        for i in range(at, min(n, at + ln)):
            out[i] += a * (rnd.random() * 2 - 1) * (1 - (i - at) / ln)
    return out
def write(path, x):
    with wave.open(path, "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(R)
        w.writeframes(b"".join(struct.pack("<h", max(-32767, min(32767, int(v * 12000)))) for v in x))
n = 45 * R; d = int(0.7 * R)
bed = bursts(1, n + d, 260, 1.0)
write(sys.argv[1] + "/m.wav", [b + s for b, s in zip(bed[d:], bursts(2, n, 160, 0.5))])
write(sys.argv[1] + "/d.wav", [b + s for b, s in zip(bed[:n], bursts(3, n, 160, 0.5))])
PY
ffmpeg -hide_banner -loglevel error -f lavfi -i 'color=size=64x64:rate=25:duration=45' -i "$TMP/m.wav" \
  -map 0:v -map 1:a -c:v libx264 -bf 0 -g 25 -c:a ac3 -shortest "$TMP/match_main.mkv"
ffmpeg -hide_banner -loglevel error -f lavfi -i 'color=c=red:size=64x64:rate=25:duration=45' -i "$TMP/d.wav" \
  -map 0:v -map 1:a -c:v libx264 -bf 0 -g 25 -c:a eac3 -metadata:s:a:0 language=pol -shortest "$TMP/match_dub.mkv"
"$TMP/ext" "$TMP/main.mkv" "$TMP/other.mkv" "$TMP/dts.mkv" "$TMP/short.mkv" "$TMP/other.mp4" "$TMP/aac44.mkv" \
  "$TMP/main60.mkv" "$TMP/long.mkv" "$TMP/long_nocues.mkv" "$TMP/match_main.mkv" "$TMP/match_dub.mkv"
