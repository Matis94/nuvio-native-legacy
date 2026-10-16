# Experimental Dolby Vision P7 to P8.1 on LG webOS

This patch extends the MKV Dolby Vision path with a separate
**Dolby Vision P7 to P8.1 (experimental)** setting, enabled by default in this
test build. With that option enabled, a P7 HEVC MKV can use a stateless libdovi converter:
FFmpeg demux -> HEVC Annex-B -> discard EL -> convert RPU -> LG native decoder.
There is no software video decoding, full-film download or external PC server.
The original **Dolby Vision in MKV (experimental)** setting controls P5/P8
independently. Leave it off to use the TV's native P5/P8 playback while keeping
P7 conversion enabled. Other platforms are unchanged.

P7 FEL's enhancement image is discarded, not reconstructed into the base layer.
This is a compatibility conversion, not lossless FEL playback. MEL/FEL metadata
are converted using libdovi mode 2. Invalid RPU data fails the experimental path
and lets its existing HDR10 fallback run. Formats with separate BL/EL tracks,
DRM and raw Blu-ray playlists are outside this implementation.

The existing audio restrictions still apply. Prefer an AC-3/E-AC-3 audio track
for the experimental MKV DV path. This change does not implement TrueHD/Atmos
conversion or additional audio passthrough support.

## Build

Rebuild the SDK image from the updated `tools/Dockerfile` (Rust 1.88.0 and its
ARMv7 soft-float target are added), then use the normal recipe:

```sh
docker build --platform linux/amd64 \
  --build-arg SDK_URL=https://github.com/openlgtv/buildroot-nc4/releases/download/webos-a38c582/arm-webos-linux-gnueabi_sdk-buildroot-x86_64.tar.gz \
  -t nuvio-webos-sdk tools/
NUVIO_BUILD_PLATFORM=linux/amd64 NUVIO_DV7=1 \
  NUVIO_ARES_PACKAGE=/path/to/ares-package bash tools/arm.sh --ipk --build
```

Keep the project's normal `local.properties` configuration. No account/server
configuration is provided by this patch. `NUVIO_DV7=0` builds the old P5/P8 path
without converting P7. The SDK can also omit Rust with `--build-arg NUVIO_DV7=0`;
then pass `NUVIO_DV7=0` when building the app too.

`tools/dv7/Cargo.lock` pins every Rust dependency. The metadata library is
`dolby_vision` 3.3.2 (MIT). Its transitive notices and provenance are copied to
`deploy/app/licenses/dv7` by the ARM build.

## Full source filenames

Both **Source text: from the addon** and **from nuvio** wrap the focused source's
full filename, including names without spaces. Other sources stay compact.
The formatter also displays description lines beyond the previous five-line
limit. Its 95-byte per-piece buffer truncation has been removed. The limit is
now the source data already stored by the app (512-byte filename, 2048-byte
description), not the viewport width. Truncation already done by an add-on
cannot be recovered by the client.

## Validation

```sh
bash tests/textwrap.sh
bash tests/dv7.sh
NUVIO_DTS_ROOT=/path/to/host/ffmpeg bash tests/dv7_engine.sh
bash tests/fontes_nome_completo_shot.sh /tmp/fullnames
```

The new engine test uses synthetic HEVC/E-AC-3 images/audio plus MIT-licensed
RPU fixtures; it verifies MKV detection, P8.1 output, EL removal, audio
passthrough, repeated seeks and rejection of corrupt metadata. These tests do
not establish Dolby Vision picture quality or sustained 4K performance on a TV.
The full app has also been compiled and linked for the LG ARM target; both
source-list layouts were rendered on Linux. Physical LG testing is still needed.

On a TV, verify the actual Dolby Vision picture mode, correct colors, playback
with a high-bitrate P7 sample, seeking, audio switching and HDR10 fallback.
The app's DV badge alone does not prove that the physical TV is displaying DV.

## 2.0.6 source port

The fork port is based on commit c2e50a9325719ffae85005c0542338f6b44878c3.
The original 2.0.5 IPK was reported working on the owner's LG C5. The 2.0.6
source changes require a fresh ARM build and physical TV validation; see
../buffer64-ram/PORTING.md. No changes from the later DTS/DV detection analysis
are included in this feature port.
