# Starfish packet adapters

`src/dts/adapter/` bridges `nuvio_dts_adapter_v2` to LG firmware via
`dlopen`/`dlsym`. Missing symbols leave fallback unavailable. No firmware library
is bundled. `tools/arm.sh` builds/stages both C++ string ABIs automatically:
`dts-starfish-webos3.so` (legacy) and `dts-starfish-webos4.so` (modern).

Standalone build: `bash tools/build-dts-pipeline.sh --output build/dts-pipeline`.
Defaults: Podman, `localhost/nuvio-webos-sdk:latest`; override with
`NUVIO_CONTAINER_RUNTIME`/`NUVIO_DTS_SDK_IMAGE`. Runtime lookup uses `lib/`
beside the executable; `NUVIO_DTS_ADAPTER_DIR` supports isolated fixtures.

The worker serializes controls, feeds preroll after Load, and plays once both
streams and loadCompleted are ready. Production seeks recreate the pipeline.
The isolated `NV_NATIVE_SEEK_TEST` application build also tries the public
`StarfishMediaAPIs::Seek` on healthy DTS/DV playback with unchanged output audio
format. The native protocol receives decimal milliseconds and acknowledges with
`SEEKDONE`; the adapter suppresses old frame clocks until that acknowledgement
and reapplies play/pause on the worker after fresh A/V preroll. Missing/refused
Seek or lack of progressing presentation clocks triggers one fresh-Load fallback
and disables native reuse for that session. The validated demux and bounded HTTP
cache survive healthy seeks and audio changes, including the fallback.

Protocol reference: LG's [webOS Chromium BUFFERSTREAM implementation](https://github.com/webosose/chromium84-temp/blob/df04aa361ad493d47003a80ebfa3034dfc9de6e0/src/media/neva/webos/media_platform_api_webos_smp.cc)
(`Seek`, `SEEKDONE` and `MakeLoadData`). The adapter uses an optional exported
method; it does not inspect firmware player/pipeline layouts or scan memory.
Actual decoder continuity and Dolby Vision signalling still need LG TV testing.

 Feed copies
packets: `Ok` releases them, `BufferFull` retries, and `Pending`/unknown replies
fail while retaining bytes through destruction. Ownership follows
[Kodi's reference](https://github.com/xbmc/xbmc/blob/c94ca95c20ce273cc76bda6e2ab9cadaa8504b20/xbmc/cores/VideoPlayer/MediaPipelineWebOS.cpp)
and requires physical-firmware verification.

`bash tests/dts_pipeline.sh` tests both ABIs against a fake native library.
See the [DTS guide](../../docs/features/dts/README.md) for limits and licenses,
and [DTS Debug builds](DEBUG.md) for a separate test app.

The v6 TV test primes one borrowed engine packet while the existing DV player
is paused, before issuing native Seek. It uses the existing pending-frame slot,
not another packet queue. A four-second DV no-progress guard allows bounded
post-seek preroll; DTS retains its two-second guard. Startup audio selection
reuses the validated demux and makes one final resume seek, with full reopen on
reconfiguration failure. Confirmed native DV MP4 with AC-3/E-AC-3/DTS audio also
tries the packet path in this test build; missing/unsupported DV metadata or
ambiguous audio mapping returns to URI playback. AAC/TrueHD MP4 stays on URI.
The UI holds the selected seek position through stale clocks/keyframe preroll,
with a bounded timeout; the native clock itself is not rewritten.
