# App diagnostics export — 2.0.6 diagnostics test 4

This build preserves the previous buffer test 3, DTS, independent Dolby Vision
P7 → P8.1 setting, P5/P8 playback, full focused filenames in both modes,
64/128/256 MiB buffer choices, buffer display and optional Green-button RAM overlay.
It adds diagnostics, not a new fix for the intermittent source switch.

## Use on the LG TV

1. Install the diagnostics test 4 IPK over the current app. Its ID and version
   remain `space.nuvio.native.legacy` / `2.0.6`.
2. Reproduce the playback problem. Keep the app running after the source switches.
3. Press **Red** to open the app log, then **Blue** to open **Export diagnostics**.
   Desktop equivalents: F9, then F10. Green still toggles the RAM overlay.
4. Scan the QR code with a phone on the **same local network**, or type the shown
   URL into a computer browser. Keep the QR open on the TV while downloading.
5. The page has all available stdout/stderr categories, a text filter and
   **Download .txt**. Useful filters: `playback-diag`, `worker-rate`, `dts-range`,
   `movie-buffer`, `failover`, `error`. Clear the filter to read everything.
6. Send the downloaded text here or attach it to a GitHub issue/PR comment.
   Export does not post to GitHub automatically and contains no GitHub token.
7. Back closes the QR, stops the LAN server and deletes the temporary snapshot.
   A second Back closes the log panel. The address expires after five minutes;
   press OK on the expired QR card to capture a fresh snapshot and address.

The page is a **frozen snapshot**, not a live feed. Reopen export for newer lines.
It reads the current app session, all categories, up to the last **16 MiB** of
input and at most 16 MiB of sanitized output. It reports older omitted bytes.
An incomplete first tail line and lines exceeding 64 KiB are omitted completely.
The TV sanitizes the file in small buffers; it never holds the whole log in RAM.
The phone/computer handles searching. A huge file can take longer to open there.
The optional Copy button may be unavailable on plain HTTP; use Download .txt.

## Privacy and network behavior

- The original app log is never served by the export endpoint. The temporary
  sanitized snapshot is private (0600) and removed when the QR closes.
- Complete URLs, recognized credential/account fields, cookies, passwords,
  tokens, PINs, email/IP/MAC identifiers, paths, JSON payload lines and long
  opaque strings are masked. Sensitive continuation blocks are omitted.
- Masking is conservative and **best effort**, not a guarantee for every unknown
  log format. Review the text before sharing it publicly. Some diagnostic text
  is intentionally removed along with sensitive data.
- The on-screen log uses the same line sanitizer. The app's private raw log file
  and the existing upstream developer “Send now” workflow are separate from
  this new LAN export. Use **Blue / QR** to obtain a sanitized downloadable file.
- Only GET/HEAD work in export mode. No CORS; Host and Origin checks, a random
  token, five-minute expiry and wrong-token limits reuse the existing phone
  input server. No third-party website, assets or upload backend are involved.
- The app opens a LAN listening socket only while the QR is open. A stalled
  downloader is cancellable with Back. Network isolation/guest Wi-Fi or webOS
  firewall rules can prevent access; LAN access still needs testing on this LG.

## Reading playback events

`[playback-diag]` writes a numeric snapshot at source generation changes and
roughly every five seconds while a source is active, independent of the RAM
OSD toggle. Fields include the route (`native-uri`, `dts-conversion`,
`dv-conversion`), position, native buffer end, playback/ready/failure state,
buffering duration, DTS state, detected DV profile, enabled conversion options,
unread payload bytes/capacity, RSS/peak and system available/total KiB.
Driver and GPU allocations may be outside process RSS. Native buffer seconds
and the estimated download timeline are different measurements.

`native-error` records the numeric code and network classification without
embedding the native event's JSON or stream URL. `failover` records the source
index, position, buffer values and trigger before retry/switch cleanup:

| reason | Trigger |
| --- | --- |
| 1 | The video backend or player reported failure. |
| 2 | The source opening deadline expired. |
| 3 | Buffering exceeded the existing VOD timeout. |

Existing `[dts]` worker-rate, stall/recovery and `[dts-range]` network events are
included. These and `movie-buffer`/`playback-diag` appear in the TV Video category.
Five-second sampling adds small periodic reads/formatting, not per-packet logs.
A long TV playback test is still required to diagnose the intermittent switch.

## Validation

- Redaction fixtures: credentials/URLs/personal identifiers, numeric diagnostics,
  multiline secrets, oversized lines, tail boundaries and private permissions.
- Actual local HTTP: viewer/download/HEAD, snapshot immutability, POST rejection,
  Host/Origin/token checks, fresh tokens, expiry and cancelled stalled transfer.
- Existing phone text-entry tests remain passing.
- New export/redaction tests passed ASan/UBSan (host leak checking disabled).
- The focused buffer teardown regression passed, preserving genuine new errors.
- All 338 ARM C sources compile and link with the existing SDK and static
  FFmpeg/libdovi/libass/P2P libraries. No new dynamic dependency is introduced.
- CI had no LAN route; HTTP tests used an explicit compile-only loopback flag,
  which is absent from the IPK. No physical TV/UI or long-playback test here.
