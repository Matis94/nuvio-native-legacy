# Early blocking-range retry and source reuse — 2.0.6 test 8

The owner's test-7 log confirms recovery of a 1 MiB interval after a 15 s timeout,
with the remaining prefetch retained and no timestamp pad/reload. However, the
consumer waited 4.086 s for that interval and fed-ahead audio/video shrank to
60/120 ms. The next five-minute sample was stable. A separate startup delay
came from reopening/probing the entire source when seeking to the saved position.

## Early retry

After the engine has emitted an audio/video packet, mark ONLY the range which
an active consumer is waiting for as urgent. During the first two of the existing
three outer fetch attempts, libcurl's progress callback may abort that request
when it has made no download progress for at least **2500 ms**. The callback can
run only about once a second during idle periods, so this is a threshold, not an
exact deadline; the host fixture detects a stalled request at roughly 3 s.

Body progress resets the timer. A slow but continuously progressing response is
not cut off merely because total download time exceeds 2.5 s. Speculative future
ranges are not marked urgent, and container opening/probing does not enable the
guard before the engine has emitted media. Open/seek resets that live marker.
The third attempt omits the early guard and retains the existing 15 s per-hop
request timeout, allowing a persistently slow server a final normal attempt.
Early-stall retry has no added backoff; ordinary failures keep test-7 waits.
Cancellation takes priority. For DV a deliberately aborted idle request does not
invalidate the cached final address or force add-on re-resolution by itself.

Keep the same range offsets and response/body validation. An incomplete response
is discarded before retrying; it never reaches the decoder. Other prefetched
ranges remain allocated. No extra lane or payload budget is added; the native
URI sequential buffering route is unchanged. This cannot guarantee smooth
playback through every network/server outage.

## Reuse the open source for an explicit seek

LG BUFFERSTREAM still gets a new native pipeline/Load with the new target and
codec preroll: flushing the old decoder alone is unsafe on these TVs. A healthy
explicit seek now uses `dts_engine_seek` on the existing container instead of
reopening/probing the entire source. It retains the exact selected stream and
core mode and resets demux/decoder/resampler/encoder state through the existing
seek implementation. Failed engine seeks fall back to one full source reopen;
Stop remains cancellable. Detected native stalls, timestamp-error recovery,
core fallback and audio-track changes retain full reopen behavior.

This reduces repeat source preparation for the saved-position seek, but does
not eliminate the separate native Load, first-open probing or every startup
network request. Actual speedup on LG requires a device run.

## Diagnostics and install

New `range-stall` logs show the interval's start, idle milliseconds and received
bytes before deliberate early abort. The following `range-http` may show curl 42
(aborted callback); `range-retry` / `range-result` distinguish recovery from Stop.
`source-seek-requested` / `source-seeked` identify reuse; `source-seek-reopen`
identifies a failed source seek falling back to a full reopen. Test-7 range and
older timestamp diagnostics remain numeric and sanitized.

Install `space.nuvio.native.legacy_2.0.6_arm-dts-early-test8.ipk` over test 7.
Title: **Nuvio Legacy (DTS test 8)**; app ID/version stay unchanged. Compare the
same source, DTS track and **128 MiB** budget. Check saved-position startup,
forward/backward and paused seeks, pause/resume, lip sync and Stop. Play beyond
previous slow intervals. Without restarting, press **Red -> Blue**, scan the QR
on the same network and download the text log. Header: **DTS early retry test 8**.

All earlier P7 -> P8.1 / separate P5/P8 controls, DTS/core/AC3, full filenames,
64/128/256 MiB buffers, optional Green-button RAM overlay, sanitized LAN export,
bounded audio padding and bounded native reload recovery remain included.

## Validation

Extend real HTTP faults with a stalled pre-header response, a stalled partial
body, repeated stalls with a slower successful final attempt, a continuously
progressing slow transfer, an idle speculative response, unguarded startup and
cancellation during a blocked live read. Assert exact bytes and retry counts.
Host stall cases recover in roughly 3 s versus 8 s without the new guard; slow
progress/speculative/startup cases complete without an early retry. Existing
503/truncation/wrong-range/auth/DV/cancellation tests and byte-identical real
H.264/DTS-to-AAC conversion remain. Guard/real-conversion tests also run with
ASan/UBSan (host leak checking disabled).

51 controller scenarios cover fresh native Loads, preserved pause/subtitles/
volume/track state, engine reuse, failed-seek full-reopen fallback and existing
bounded error recovery; controller tests also run ASan/UBSan. The new early-read
and source-reuse regressions fail against test 7. Timestamp/encoding, bounded
prefetch, numeric redaction and HTTP/TLS/redirect validation tests are checked.
ARM compile/link retains the same dynamic dependencies and maximum glibc 2.12.
The owner's source and physical LG are unavailable in the build environment.
