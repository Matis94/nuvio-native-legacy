# DTS / DV range recovery — Nuvio Legacy 2.0.6 test 7

The owner's test-6 log records two accepted audio pads of 244.667 and 246 ms,
without a timestamp reload. Nearby network requests time out after 15 s, cached
payload falls from roughly 73 to 3 MiB and later 41 to 0.9 MiB, and fed-ahead
audio shrinks to 0.26 s. These observations suggest a transport problem but do
not prove that every timestamp gap or visible interruption has that cause.
Some native video messages were masked by the existing export sanitizer.

## Change

Retry a failed HTTP byte interval inside its existing range-worker slot BEFORE
returning an I/O error to libavformat. The start/end offsets do not change; good
later slots stay allocated while that interval is retried. Keep four existing
worker lanes, current range sizes, payload budget and response validation.
Truncated bodies and mismatched Content-Range responses are rejected; their
partial data never reaches the decoder. A successful retry provides the exact
original bytes, so transport-induced missing samples can be avoided rather than
requiring a silence pad afterward.

There are at most THREE outer fetch attempts per job, with cancellable waits
of 150 and 300 ms between attempts. Retry unknown/network failure status, failed
206 validation, failed redirects, HTTP 408/425/429 and 5xx. Other statuses such as
403, 404 and 416 are not blindly retried. The existing DV direct-link/add-on
resolution fallback remains inside a fetch attempt and can issue additional
requests. Existing per-request 15 s timeouts and redirect bounds remain; this
is not a new 15 s total deadline for the whole job. A persistent error still
reaches the prior error path after this finite budget. Stop/seek cancellation
wakes retry waits and cancels active transfer callbacks.

Actual seeks outside the active cache and missing/short-range realignment keep
the previous reset behavior; test 7 logs these resets. The new retry path itself
does not clear the queue. Native URI sequential buffering is unchanged; this
change applies to the range reader used by DTS/DV conversion and demux routes.

Test-6 bounded silence insertion and test-5 bounded reload remain fallback
mechanisms. This build cannot reconstruct samples genuinely absent from a file
or guarantee uninterrupted playback through a prolonged server outage.

## Numeric export diagnostics

- `range-http`: requested interval, curl/HTTP result, received partial bytes,
  header validity, overflow and response interval/total. No URLs/credentials.
- `range-retry`: failed attempt number and interval being retried.
- `range-result`: total outer attempts, success, byte count and elapsed time.
- `range-read`: slow (>=500 ms) or failed consumer reads, with total completed
  bytes and the contiguous completed bytes available from the current position.
  Cached bytes beyond a missing interval cannot immediately feed playback.
- `range-reset`: reset reason (`seek` or `missing`), position, discarded completed
  payload and active requests. Enables correlation with sudden buffer drops.

Existing timestamp diagnostics remain. Redaction tests verify that typical
numeric range diagnostics survive export; masking remains best effort.

## Install and test

Install `space.nuvio.native.legacy_2.0.6_arm-dts-range-test7.ipk` over test 6.
The app title is **Nuvio Legacy (DTS test 7)**. App ID/version stay unchanged.
Use the same source, DTS track, start position and **128 MiB** for comparison.
Play beyond the previous interruptions, checking sound, picture and lip sync.
Also check pause, seek and Stop. Press **Red -> Blue**, scan the QR from the
same network and download the text log BEFORE restarting the application.
The export header identifies **DTS range recovery test 7**. Include logs even
if the interruption disappears, so accepted retries can be distinguished from
pads, reloads and a run without network failures.

All earlier custom features are retained: P7 -> P8.1 and separate P5/P8 controls,
DTS/core/AC3 options, full focused filenames, 64/128/256 MiB buffers and optional
Green-button RAM overlay. This test does not increase the configured buffer.

## Validation

Real HTTP fault injection covers one/two 503 failures, truncated 206 bodies,
wrong response offsets, persistent 503, non-retried 403, cancellation during
backoff, seek cancellation and ~4 MiB DV requests. Tests check exact bytes,
retry counts and bounds, unchanged pointers to good later buffers, no repeated
fetches of good intervals, credentials and forward consumption without holes.
A regression scenario fails against the test-6 engine.

A real H.264/DTS MKV fixture produces byte-identical AAC audio and video output
after injected 503, truncation and wrong-range responses versus local transport.
The fixture also exercises AC3 5.1, repeated seeks and track selection. Range
fault tests and real conversion run with ASan/UBSan (host leak checking disabled).
Existing 49 controller scenarios, timestamp/encoding fixtures, prefetch/bounded
buffer cases and redaction tests are checked. ARM compilation/linking retains
the same dynamic dependencies and a maximum glibc requirement of 2.12.
The actual source and LG TV are unavailable in this build environment.
