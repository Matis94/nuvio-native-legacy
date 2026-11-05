# Native DTS Off + Dolby Vision experiment

Branch: `native-dts-off-test-2.0.8`. This experiment is isolated from
`buffer-dts-dv-test-2.0.8` and PR #749. Physical LG testing is required.

## Enable the experiment

Select **DTS output: Off (native)**, enable the existing Dolby Vision P7
conversion option, and reopen the video. P7 mapping and experimental DV MP4
remain separate opt-ins. AAC stereo remains the default DTS setting.

This branch's workflow enables `NV_DV_NATIVE_DTS_TEST` in both the application
and its two separately compiled webOS Starfish adapters. With Off selected,
the engine keeps DTS packets compressed, without its DTS decoder or AAC/AC3
encoder, even when fixed DV output is configured. The existing video path can
convert P7 to P8.1 and retain optional mapping while original DTS audio goes to
BUFFERSTREAM. Non-DV DTS continues to use ordinary native URI playback.

The adapter uses codec `DTS` and `contents.dtsInfo` with channels and frequency
in kHz. This protocol follows Kodi's primary-source webOS implementation:
https://github.com/xbmc/xbmc/blob/e7ead100967c9221ddf95568c6a7e970ed82dd5d/xbmc/cores/VideoPlayer/MediaPipelineWebOS.cpp

This supplies the missing Nuvio protocol support; it does not establish that
a particular LG firmware or DTS Restore Plus installation accepts DTS in
BUFFERSTREAM. DTS-HD input stays unchanged, but lossless receiver bitstream
output and DTS:X object rendering are not guaranteed. There are no new frame
buffers, cache allocations or HTTP lanes.

## Failure behavior

If native DTS Load is refused or the firmware reports unsupported audio
(error 200), the worker requests the existing native URI handoff. It does not
silently transcode DTS to AAC. That fallback does not perform app P7 conversion
or mapping. Other DV failures retain their existing recovery behavior.

A firmware that accepts Load but produces silent audio requires manual testing;
there is no reliable API here to prove that the TV actually outputs sound.

## Validation

Host adapter tests cover both ABIs, exact DTS JSON, audio/video feed, pause,
seek, synchronous refusal, and rejection without the experimental build flag.
Sanitized worker tests cover selected-track resolution, original DTS startup,
seek, synchronous/asynchronous firmware rejection, defaults and policy reset.
LG controller tests cover known DTS, late MP4 discovery and unchanged SDR routing.
Real MKV/MP4 packet tests compare DTS byte-for-byte without an app decoder or
encoder, including fixed output and audio changes.

The Actions real P7 engine tests additionally exercise MEL/FEL conversion with
original DTS, mapping, fixed-output preferences and seeking. These need Rust and
the pinned FFmpeg toolchain. Host protocol tests use a mocked firmware boundary;
actual DV/DTS playback, MP4 behavior, audio switches and seeking must be tested
on LG. Existing defaults, buffer sizes, HTTP limits and TrueHD conversion remain
unchanged.

## Diagnostics QR

Diagnostics QR is already available under the log/diagnostics settings and
remains OFF by default. Enable it, reproduce the issue, open the log panel
(Blue/F10), then use its download/export QR. The frozen snapshot includes all
ordinary logs plus `[dts-native] v=1` transition records from this experiment.

Records include the effective startup policy (Off, DV, P7, mapping, MP4,
fixed-output preference), selected stream number, original codec/channels/rate,
converted DV profile/compatibility/BL/EL/RPU flags, native Load and control
callbacks, seek/audio-switch targets, failures and URI fallback. Existing
first-feed, worker-rate, HTTP and refill logs remain available. No per-frame
measurement buffers or periodic DTS-native logging are added. Logs cannot
prove audible DTS output; report whether sound was actually heard on the TV.

The export identifies this experiment and retains these numeric records while
continuing to mask URLs, credentials and private payloads. New records never
print source URLs, headers, track names, raw native JSON or packet addresses.
