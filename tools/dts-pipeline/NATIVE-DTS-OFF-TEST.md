# Native DTS Off experiment

Branch: `native-dts-off-test-2.0.8`. This is an experimental change and needs
LG hardware testing, including with DTS Restore Plus. It is not enabled on
the shared `buffer-dts-dv-test-2.0.8` branch.

The LG DTS output setting appends native Off after the existing AAC and AC3
values. Saved modes, buffer sizes, HTTP worker limits, TrueHD handling and
DV/P7/mapping settings keep their existing meanings and defaults.

Off leaves known DTS sources on URI playback. For audio discovered only after
MP4/MKV demuxing, the worker snapshots Off before opening the engine. Provisional
DTS is kept compressed while the actual selected source track is resolved;
neither the app's DTS decoder nor its AAC/AC3 encoder is instantiated for it,
even with fixed DV audio enabled. A selected non-DTS track can still use the DV
pipeline. An actually selected DTS track returns to URI playback before any
unsupported DTS BUFFERSTREAM Load or Feed.

## Current limitation

DTS Restore Plus restores the native URI/GStreamer decoder. Nuvio's current
BUFFERSTREAM adapter accepts AAC, AC3 and EAC3, not raw DTS. Native DTS therefore
cannot yet share the P7/MP4 packet-conversion pipeline. When DTS is selected with
Off, P7 conversion and mapping are not applied; normal native playback depends
on the TV/restore installation. Keeping both original DTS and converted P7 video
requires another implementation and physical LG validation. This experiment
does not claim DTS bitstream passthrough to a receiver.

## Regression coverage

- Legacy native-buffer fixtures link against explicit DTS-setting test support.
- LG AAC/AC3/Off values and Samsung migration remain checked independently.
- Controller tests cover known DTS, mapped non-DTS selection and late MP4 policy.
- Worker tests cover final-track resolution, rejection before native Load,
  unchanged default conversion and policy consumption after a rejected start.
- Real MKV/MP4 tests compare original DTS packets byte-for-byte and check that
  no app decoder/encoder exists with Off, including fixed output, seeks,
  audio changes, reopen, policy reset and cancellation.
