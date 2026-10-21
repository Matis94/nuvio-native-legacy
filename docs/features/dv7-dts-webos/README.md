# Focused webOS DV7 and DTS topic branch

Based on official master `eb520363755ece93aba8aff52396e0b624be064e`.
Only two feature groups are carried here; this branch is independent of the
owner's full custom build.

## DV7

An independent, device-local setting enables P7 MKV -> P8.1 on LG webOS.
It defaults to **Off** and is shown only when `NV_DV7_CONVERSION` is compiled.
The converter rewrites RPU metadata and keeps the base layer; EL/FEL is
discarded, without software reconstruction of FEL. Existing P5/P8 selection,
RPU validation and crash-guard behavior remain in place.

The webOS build links the pinned libdovi bridge and its dependency licenses.
The official Rust MKV core remains enabled; `external-personality` prevents a
duplicate Rust personality when both archives are linked. Standalone MKV-core
builds retain their original personality stub.

## DTS stability

- Retry the same failed HTTP byte range without discarding good prefetch.
- Abort/retry an actively awaited, stalled range early, while leaving startup,
  progressing transfers and speculative reads alone.
- Pad only bounded, short forward audio timestamp gaps; reject backward,
  oversized or sustained discontinuities.
- Bound native timeline recovery attempts and preserve pause/track state.
- Reuse a healthy open source for explicit seeks, with full-reopen fallback.
- Invalidate closing native callbacks before replacing a DTS session.

### Independent recovery setting

**DTS playback recovery** defaults to **On** on webOS builds compiled with
`NV_DTS_WEBOS_RECOVERY`. It controls failed-range retries, early retry of an
actively awaited stalled range, bounded forward-gap padding and bounded automatic
native stall/timestamp reloads. It applies to the next converter session and
works for SDR/HDR DTS sources with P7 conversion disabled as well as DV paths.

The independent P7 switch controls only image-profile conversion. Recovery Off
keeps P7 conversion available, stops the added HTTP retries/early watchdog,
rejects discontinuities without silence padding and does not automatically
reload a stalled/broken timestamp timeline. Explicit seeks (including healthy
source reuse), pause, subtitles, audio-track switching, core fallback and
credential/range validation are unchanged. Both preferences stay on this TV;
they are excluded from account sync and device backup.

This topic branch already kept recovery independent of P7. The upstream
`febd70a` integration added `allow_p7` gates to recovery; when applying this
change there, use the recovery policy instead of `allow_p7` for transport,
padding and automatic reload decisions. Never derive recovery from a DV profile.

New HTTP retry/padding behavior is enabled by `NV_DTS_WEBOS_RECOVERY`, added
by the webOS ARM build. Without this flag, failed range reads receive the
original single fetch and timestamp discontinuities retain the original
rejection. Samsung/Android builds do not opt in. The webOS controller is already
platform-gated. Numeric recovery logs are included; no QR export or RAM overlay
is added.

## Scope and validation

The official native HTTP buffer (including current upstream updates), buffer
setting, memory policy, Android cache actions, Samsung DNS changes and P2P-tail
startup safeguard are retained. No custom movie proxy, RAM overlay, filename
wrapping, general log exporter or old buffer-setting migration is included.

Host checks run in `.github/workflows/dv7-dts-webos.yml` on this branch.
They cover default/no-recovery behavior, webOS recovery, the existing native
HTTP payload/fallback, controller state, real HTTP faults and DTS conversion,
P2P startup, DV7 fixtures, settings isolation and both Rust link modes.
These checks do not replace ARM linking or physical LG/Samsung/Android tests.
No IPK is built and no upstream pull request is opened by this branch.
