# HTTP read-ahead buffer from custom 2.0.7

This branch isolates the progressive HTTP/HTTPS RAM buffer for review against
upstream 2.0.8 (`87c8b8909f0d605f1d8351147fe5afb9de233b8d`). The source is the
buffer implementation used in the custom 2.0.7 build, separated from its
formatter, Dolby Vision, DTS recovery and UI changes.

## Behavior

- The existing 64/128/256 MiB setting controls a single-connection RAM ring.
- A loopback HTTP server serves the buffered bytes to the normal LG player.
- Recheck `MemAvailable` at load time using the unchanged official RAM policy:
  retain the 100 MiB floor and 4 MiB allowance; reduce to 128/64 MiB or Auto
  when necessary. There is no option to bypass this check.
- Disable custom native `bufferControl` when the RAM proxy is active, avoiding
  a second requested 64/128/256 MiB native buffer on top of the RAM ring.
- Auto, live modes, known HLS/DASH URLs, loopback URLs and unsupported origins
  retain their existing native playback/fallback paths.
- Cancel the RAM reader and join its workers when the native session closes.
- The ordinary buffered rail can include an estimate from measured queued
  bytes and average file bitrate; VBR means this is not an exact timestamp.

## Player presentation

`src/player.c` is unchanged. This branch adds no buffer MiB counters, status
labels, markers, RAM overlay or other player widgets. Existing settings,
formatter behavior and DTS/Dolby Vision policies are unchanged.

## Tests

- `bash tests/moviebuffer.sh` — actual HTTP fixtures for all three budgets,
  exact byte comparisons, seeks, concurrent ranges, header/redirect handling,
  playlists, unsupported origins and bounded cancellation. Requires the minimal
  host FFmpeg build (`NUVIO_DTS_ROOT`).
- `bash tests/video_moviebuffer.sh` — ordinary buffered rail, converter
  isolation, fallback and shutdown after session invalidation.
- `bash tests/video_http_buffer.sh` — native payload and official RAM clamp.
  Its old 192 MiB-floor expectations are aligned with the existing 100 MiB floor.
- `bash tests/video_http_buffer_build.sh` — compile the touched webOS units.
- `bash tests/video_acb_lg.sh` — existing native load/dispatcher regression.
- `tests/buffer_tamanho.c` — unchanged official safety thresholds.

The branch is for code review. A physical LG test of this port to upstream
2.0.8 is still needed; the custom 2.0.7 implementation was tested separately.
