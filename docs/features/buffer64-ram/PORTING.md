# Port of the working LG feature set to 2.0.6

Base: `c2e50a9325719ffae85005c0542338f6b44878c3` in
`Matis94/nuvio-native-legacy`, targeting the fork's `master` branch.

## Included behavior

- Independent P7-to-P8.1 MKV switch, separate from P5/P8 native/experimental selection.
- 64/128/256 MiB compressed movie download buffer and System (off).
- Progressive HTTP(S) caching for native movies as well as DTS/DV range reads.
- Ready MiB/capacity and estimated buffered time on the player bar.
- Optional RAM overlay, persistent GREEN-button toggle, existing RED action retained.
- Full focused filenames and UTF-8 wrapping in both source-text modes.

HLS/DASH, live and existing loopback/P2P paths retain their existing buffering.
P7 conversion discards EL/FEL; it does not reconstruct FEL. The default remains
64 MiB, RAM overlay off, and the separate P7 conversion on.

## Adaptation to the new base

Append the three local settings after all existing 2.0.6 settings to preserve
persisted IDs. Retain the newer Samsung remux/streaming reader and ring APIs.
Keep the source-list text cache from #479, with separately owned parsed lines
so cache-index reallocations cannot invalidate text pointers. Retain reusable
text textures when focus changes; wrap long names with the same measurement
and drawing logic. Extend the existing formatter regression fixture to cover
long filenames, cache growth, Linux GLES rendering and measurement.

The later analysis of intermittent DTS/DV detection did not introduce any
production fixes here. Its probe/retry/bind behavior is unchanged by this port.

## Verification scope

The original 2.0.5 custom ARM IPK was compiled and reported working on the LG C5.
This PR is a source port to 2.0.6, not a new compiled TV package. Host tests do
not establish physical TV playback, memory headroom or long-playback stability.

Current checks and their results are listed in the pull request. FFmpeg 7.1.5
was built with the repository's minimal host configuration. The DTS fixture
used available libx264 with B-frames disabled instead of missing libopenh264.
ASan/UBSan wrapping tests ran with LeakSanitizer disabled because the runtime
does not allow its process inspection.

One existing test, tests/ajustes_dts.py, still asserts that no DTS setting exists
in the catalog; its first assertion also fails on the unmodified 2.0.6 base,
which contains AJ_DTS_AC3. This unrelated assertion was not changed or counted
as a passing check.
