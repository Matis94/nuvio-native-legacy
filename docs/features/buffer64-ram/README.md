# LG test build: selectable movie buffer and playback RAM display

Ported to Nuvio Native Legacy 2.0.6, commit
`c2e50a9325719ffae85005c0542338f6b44878c3`. The original TV-tested custom
2.0.5 build used base `5d2f8c8c99113e416e1fb06f8b60da1d13635e85`.
Includes P7-to-P8.1 conversion and full source filename fixes. The 2.0.6 port
preserves upstream settings and Samsung streaming/remux support, and adapts
full-name wrapping to the parsed source-text cache (#479). See PORTING.md
for validation specific to this source port.

## Settings and remote

Open **Settings > Playback > More options > Sound and video on this TV**.
The exact section names follow the selected application language.

- **Movie download buffer (test):** 64 MiB (default), 128 MiB, 256 MiB,
  or System (off). Changes apply when opening the next playback session.
- **Show RAM usage during playback (test):** off by default. The GREEN remote
  button toggles the same saved preference during full-screen playback.
  Desktop test builds also accept G. Holding the key does not repeatedly toggle.
  The RED button retains its existing log action.
- **Dolby Vision P7 to P8.1 (experimental):** enabled by default, independent
  of **Dolby Vision in MKV (experimental)**, which controls P5/P8.
  To keep native P5/P8 playback while converting P7, turn the existing P5/P8
  option off and leave the new P7 option on.

The three new preferences are local to this device and are not synchronized
with an account. Existing settings retain their indexes and meanings.

## Download buffer coverage

The selected limit applies to the shared DTS/Dolby Vision HTTP range reader
and to ordinary progressive HTTP(S) movies, including files without DTS or DV.
The latter use a loopback HTTP byte cache; video and audio remain unchanged
and are decoded by the native player. Authenticated requests retain the
original source headers. Seeking and closing discard obsolete cached ranges.

HLS/DASH playlists, live playback and existing loopback/P2P sources retain
system or their existing buffering. Origins without valid finite HTTP Range
responses are redirected back to the native URL before media bytes are sent.
Opaque HLS/XML manifest URLs are detected at the first read and also redirected.
Adaptive/DRM playback is not given a new custom download cache.

System (off) disables the ordinary URI cache. DTS/DV processing still requires
its internal reader and uses the 64 MiB default in that mode.

The limit counts compressed range payload, including the active read cache;
it is not a limit on the app's total RAM. Four HTTP workers are retained.
Ordinary DTS requests use 1 MiB ranges; DV and the raw URI cache use ranges of
4 MiB minus one byte. Additional memory is needed for network/TLS, decoders,
demuxing, graphics, subtitles, packet copies and allocation overhead. Small
files and origins returning short ranges can fill less than the selected limit.
With approximately 260 MiB system memory available, the 256 MiB option can
exhaust the remaining headroom; 64 MiB remains the starting default.

## Playback indicators

The player controls show **Buffer: used / limit MiB** for the custom cache,
or **Buffer: system** for other paths. The gray timeline combines the native
buffer endpoint with an estimate based on downloaded bytes and average file
bitrate. This time estimate can vary with variable bitrate files; the numeric
MiB count measures actual ready payload.

The optional top-left RAM display updates once per second, including when
player controls are hidden:

- **RAM app (RSS):** resident process memory.
- **peak:** process-lifetime RSS high-water mark, not just this movie.
- **System available:** available/reclaimable system RAM and total Linux RAM.

Disabled overlays do not sample /proc or draw. Unavailable values are marked.
App RSS does not include every GPU, decoder or driver allocation. Mini-player
and trailers do not display the overlay.

## Preserved changes

P7 MEL/FEL conversion copies base HEVC video, converts RPU with libdovi mode 2,
and discards the enhancement layer. It does not reconstruct FEL. Disabling
conversion leaves playback to the TV. P5/P8 selection is independent.
Full focused filenames and UTF-8-safe wrapping work in both source-text modes.

## Validation and build

The original 2.0.5 ARMv7 application was compiled with P2P, ASS subtitles,
FFmpeg and libdovi, with runtime assets/adapters from the working IPK. This
2.0.6 pull request contains source changes only; it does not publish a new IPK
or service credentials. Build/retest an ARM package before installing this port.

Tests cover all three URI budgets, payload equality, authenticated HTTP,
range requests, parallel clients, pause/resume, seeking, cancellation and
unsupported-origin fallback. DTS/DV engine tests, independent profile selection,
settings persistence, RAM toggling and full filename rendering are also checked.
Desktop renderer captures verify the buffer label and green-button handler.
The settings catalog test passes on the 2.0.6 port; see PORTING.md for
other existing test limitations.

Use the pinned SDK and configured local.properties with the updated
`tools/arm.sh --ipk --build` recipe and DV7 prerequisites described in the
DV7 README. New src/*.c files are discovered automatically. The default and
maximum buffer budgets are in src/dts/range_budget.h.

No sustained playback or memory measurements were performed on an LG C5.
The TV test must establish whether the reported stutter improves, whether
native URI playback accepts the cache, and how much RAM remains available.
