# DTS timestamp recovery — 2.0.6 test 5

A diagnostics test 4 log captured `DTS timestamp discontinuity requires reload`
at about 259.6 seconds. Just before failure, the DTS-to-AAC stereo path had
124.2/128 MiB cached and approximately 362 MiB system RAM available. The old
controller treated the mismatch as terminal and handed the same source to the
native player, ending software DTS conversion. The origin of the timestamp
mismatch itself remains unconfirmed.

## Behavior

A timestamp discontinuity now requests a fresh conversion/native Load at the
latest presentation clock, preserving the selected audio stream, current
DTS/core mode, DV mode, pause, volume and subtitle selection. Fed-ahead packet
positions are not resume targets. Old native shutdown callbacks are suppressed
both during reload and terminal teardown.

At most two recovery attempts are allowed without 30 seconds of forward
presentation progress since the last attempt. Reopening time and pauses do not
reset the budget. User seeks and audio-track changes start a fresh budget.
Persistent errors still reach the existing terminal fallback, avoiding an
endless reload loop. A failed reopen and Stop remain cancellable.

This is controlled recovery: a short pause/reload can be visible. It does not
promise seamless repair or successful playback across a persistent source gap.
The existing 100 ms discontinuity threshold remains unchanged; timestamps are
not silently ignored, samples are not dropped and silence is not inserted.

New numeric logs (without URLs or credentials):

- `timestamp-gap`: incomingMs, expectedMs, deltaMs, fifoSamples, delaySamples,
  frameSamples, inputRate and targetMs.
- `timestamp-recovery-request`: resume position, attempt, core, stream and DV.
- `stage=timestamp-reload`: a replacement Load has been prepared; subsequent
  worker/native clock progress is needed to confirm playback has resumed.
- `stage=timestamp-recovery-exhausted`: the retry limit has been reached.

## Install and test

Install `space.nuvio.native.legacy_2.0.6_arm-dts-timestamp-test5.ipk` over test 4.
The app title is **Nuvio Legacy (DTS test 5)**. App ID/version remain unchanged.
Use the same file/audio track and keep **128 MiB** for the first comparison.
Play from before the failing 4:20 point, or the same saved position as before.
Check whether conversion continues after any brief reload. Test pause/resume,
seeking and closing playback too.

Without restarting the app, press **Red → Blue**, open the QR on a phone or
computer on the same network and select **Download .txt**. Send the log even
if playback continues: the exact timing delta is useful. The snapshot header
identifies **DTS timestamp recovery test 5**. Filter for `timestamp`,
`playback-diag` or `worker-rate`, or clear the filter for all categories.
Automatic masking is best effort; review the export before public sharing.

Previous custom features are preserved: P7 → P8.1 and separate P5/P8 options,
DTS, full focused filenames, 64/128/256 MiB budgets, buffer indicator, optional
Green-button RAM overlay, buffer-test-3 fixes and sanitized LAN log export.
The input buffering algorithms/budgets are unchanged in this test.
See `docs/features/diagnostic-export/README.md` for export behavior and limits.

## Validation

49 controller scenarios, including nine new timestamp cases: successful
recovery, bounded retries, progress-based reset, reopen failure, cancellation,
paused recovery, DV preservation, user seek and track-change reset. Existing
pause, seek, subtitles, core/CPU recovery, stall and backpressure cases remain.
The new recovery test fails with the previous controller.

A real FFmpeg resampler fixture checks ±100 ms boundaries and larger forward/
backward gaps. Controller/resampler suites also run with ASan/UBSan (host leak
checking disabled). Redaction regression and full ARM compile/link are checked.
The IPK retains the prior runtime/services and dynamic dependencies; maximum
required glibc is 2.12. Physical-TV recovery and the actual failing file still
need an owner test; the source is unavailable in this build environment.
