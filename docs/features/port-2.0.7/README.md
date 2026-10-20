# LG custom features on official 2.0.7

Base: official `v2.0.7`, commit `5eddbe92f272a6f23d459b15354c6657bbf133d1`.
Custom source: `7a38138f1643302123ab9160c5cfcb3921a01097` (working 2.0.6 test 8 plus UI/log fixes).

## Integration

- Keep the official Auto/64/128/256 MiB setting, free-memory clamp and backend API.
  Display one setting with the official 2.0.7 name and values (`Tamanho do buffer`
  / **Buffer size**, Auto/64/128/256 MB), using the upstream translations. Auto keeps native LG buffering for
  ordinary HTTP files and the official default for the DTS/DV range reader.
- Extend that same effective setting to the native HTTP(S) movie/episode loopback
  cache. It uses one continuous upstream transfer. HLS/DASH, live and existing
  loopback/P2P routes retain their existing buffering.
- Convert the saved custom `movieDownloadBufferLocal` indexes
  0/1/2/3 (64/128/256/System off) to official indexes 1/2/3/0.
  Subsequent saves write only `bufferPlayerLocal`. Auto is the replacement for
  the old native-cache-off choice.
- Retain the official 64-slot ~4 MiB range layout for requested DTS/DV buffers.
  The separate active range is part of its existing memory reservation.
  The MiB indicator reports actual payload capacity, including that active range.
  Do not reintroduce the custom 255-slot layout or a second DTS buffer setter.
- Retain test-8 bounded timestamp recovery, small forward audio-gap padding,
  same-range HTTP retries, urgent stalled-read detection and healthy explicit
  source-seek reuse.
- Add independent P7 -> P8.1 conversion (base layer and rewritten RPU; EL/FEL
  discarded), UTF-8 full filenames in both source modes, optional GREEN-button
  RAM overlay, sanitized local QR log export and healthy-snapshot classification.
- Preserve official P8.1 RPU admission checks, version-scoped crash guard,
  PGS/ASS support, audio selection memory, low-memory cleanup/reopen protection,
  source filters/languages, multipart playback and the soft-float Rust MKV core.
- Keep DV7 and RAM overlay toggles local, including the new 2.0.7 portable
  preferences/device-backup paths. They cannot be changed through account sync.
- Link the official Rust MKV core together with the custom libdovi wrapper and
  its licenses. In the combined ARM build, the core's `external-personality`
  feature uses the personality supplied by libdovi's Rust standard library;
  standalone builds retain the official no_std stub. No duplicate-symbol
  suppression or removal of the official Rust core is used.
- The abandoned LG frame-counter experiment is excluded.

## Validation status

Host checks run in [GitHub Actions](https://github.com/Matis94/nuvio-native-legacy/actions/workflows/lg-custom-207.yml).
The local execution environment was unavailable during this port. Current PR
checks, rather than the older test-8 reports, determine host validation status.
No ARM package or TV run has been completed for this port. No IPK is included.

Checks cover official version/buffer policy, legacy setting migration,
controller recovery, real HTTP faults/byte continuity, native cache, timestamp
padding, MEL/FEL RPU conversion, redaction and source-list rendering.

Before merging, run the workflow successfully and build the ARM package with
the official 2.0.7 toolchain plus DV7. On LG test AAC/AC3 native HTTP files,
DTS, P5/P8 and P7+DTS, every buffer choice including Auto, saved-position and
paused seeks, audio/subtitle changes, PGS/ASS, Stop and extended playback.
Export a fresh sanitized log after each problematic test.

Older feature documents under `docs/features/` describe the original 2.0.6
development and hardware tests, not validation of this port.
