# DTS audio timestamp gaps — 2.0.6 test 6

Test 5 kept DTS conversion alive by reopening the source at the native presentation
clock when a timestamp discontinuity occurred. Two owner logs show successful
recovery after forward audio gaps of **202.667 ms** and **245.333 ms**, but opening
the source took about 4.9 and 7.7 seconds. Destroying/reloading the native pipeline
explains the visible black interval. The reason those audio timestamps diverge
from the decoded samples is still unknown; these logs do not establish whether
the original file, transport or decoder caused the gap.

## Test 6 behavior

The existing ±100 ms jitter tolerance remains. When a decoded audio frame is
more than 100 ms ahead, but at most **500 ms** ahead, insert the missing duration
as silence through the existing FFmpeg resampler. The input sample rate is used
for injection; queued FIFO samples and resampler delay remain part of the expected
output timeline. The encoder clock continues normally, and video timestamps and
the native player are not reset for an accepted correction. A brief quiet portion
of audio may be audible. Missing sound cannot be reconstructed.

Allow at most **four corrections and one second of inserted silence** before
30 seconds of forward decoded-media progress from the first correction starts a
new budget window. Pausing does not advance this window. Opening a new engine,
seeking, and changing the audio track start a new timeline/budget. A backward gap,
a forward gap over 500 ms, exhausted budget or injection error retains the test-5
bounded reload recovery: two attempts without 30 seconds of forward presentation
progress, followed by the existing terminal fallback if recovery remains broken.
This does not guarantee uninterrupted playback for every source.

Input buffering connections, algorithms and RAM budgets are unchanged. P7 → P8.1,
separate P5/P8 controls, DTS/core/AC3 options, full focused filenames, 64/128/256 MiB
buffers, buffer indicator, optional Green-button RAM overlay and sanitized LAN
log export remain included.

## Numeric diagnostics

- `timestamp-gap`: incoming/expected/delta milliseconds, FIFO/resampler/frame
  sample counts, input rate and seek target.
- `timestamp-pad`: accepted gap, inserted input samples, input rate, number of
  corrections and total inserted milliseconds in the current window.
- `timestamp-reject`: `backward`, `too-large`, `budget` or `inject`, then the
  existing controller recovery diagnostics.
- `stage=timestamp-reload`: a fallback replacement Load has been prepared.

These fields contain no URLs or credentials. The export header identifies
**Nuvio Legacy 2.0.6 DTS audio gap test 6**. Masking is best effort; review an
export before publicly sharing it. Export details and limits are documented in
`docs/features/diagnostic-export/README.md`.

## Install and compare

Install `space.nuvio.native.legacy_2.0.6_arm-dts-gap-test6.ipk` over test 5.
App title: **Nuvio Legacy (DTS test 6)**; app ID/version remain unchanged.
Use the same file, audio track, start position and **128 MiB** setting for the
first comparison. Play beyond the previously failing points, then check pause,
seek, audio selection and Stop. Note any black interval, silence or lip-sync drift.

Without restarting the app, press **Red → Blue**, scan the QR on a device on the
same network, and download the text log. Filter for `timestamp` to distinguish
an accepted pad from a fallback reload. Physical LG/webOS validation is required;
the owner's source file and TV are unavailable in the build environment.

## Validation

Real FFmpeg 7.1.5 fixtures reproduce both observed sample deltas with a nonempty
FIFO, verify silence followed by original samples, continued frame acceptance,
the unchanged jitter boundaries, maximum-gap and count/duration budgets, progress
reset and a new timeline after seek. Eighteen real AAC stereo / AC3 5.1 encoding
runs cover 44.1, 48 and 96 kHz input, planar float, interleaved float and signed
16-bit input, pending resampler delay, encoder draining and strictly increasing
packet timestamps. Sample totals remain within two 48 kHz samples of the source
clock plus the inserted gap, with only normal encoder-frame padding at the end.
The new fixtures fail against the test-5 engine.

All 49 existing controller scenarios, resampler/encoder fixtures and export
redaction tests pass; controller and audio suites also pass ASan/UBSan with host
leak checking disabled. The ARM build preserves runtime/services and dynamic
library dependencies; maximum required glibc remains 2.12. These host checks do
not establish how the TV displays a particular stream.
