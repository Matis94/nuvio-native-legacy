# Experimental MP4 BD REMUX test 1

Base: v17-ui, commit `85ca963f28afb9ddd0edded0e013fca579493fbc`.
Branch: `build/mp4-bd-remux-test`. The `build/playback-v17-no-qr` branch is not modified.

## Scope and settings

Settings > Playback > this TV's sound/video section:
**BD REMUX MP4 compatibility (test)**, default **Off**, stored on this TV only.
Reopen the video after changing it. Old preferences retain their IDs and keys.

Off retains the v17-ui playback policy. On retries a native startup demux error
204 only for a source recognized as MP4, once per opening. A playing source,
MKV, live source and an already attempted converter do not enter this retry.
The worker then verifies the real container metadata: exactly two HEVC video
tracks, one HDR10-compatible base and a separate P7 EL/RPU track. Unrelated
MP4, P5/P8, ambiguous tracks and encryption are rejected. A BD REMUX filename
alone is never evidence of this layout. The normal MKV P7 preference remains
independent; enabling this explicit test allows P7 for this MP4 retry only.

The retry pairs compressed base pictures with the separate track's RPU by
both DTS and PTS. Existing libdovi mode-2 conversion produces P8.1 metadata;
EL pictures are discarded without decoding, and base video is not re-encoded.
The existing audio selection/fixed-output, seeking and native reload fallback
remain in use. Missing/invalid RPU, timestamp mismatch, excessive interleave
or cancellation fail explicitly rather than advertising unverified DV output.

## Memory

No additional video decoder, native player instance or HTTP lane is added.
Pairing retains at most eight base packets sharing a 4 MiB payload budget and
64 RPUs of at most 8 KiB each. Four existing HTTP range slots (about 16 MiB)
are reserved before workers start, covering pairing storage and temporary
packet growth without raising the previous read-ahead payload budget. This
reserve applies only to the new MP4 engine, not MKV or normal HTTP buffering.

## Diagnostics

Restores the previous sanitized diagnostic QR export in this test branch.
Open the log with the red remote button, then press blue (F10 on a keyboard).
Scan the QR using a device on the same LAN and download the frozen text log.
The export expires after five minutes; Back closes it. The ordinary phone
text-input QR flow and bounded socket writes are retained.

New numeric/technical log entries use `[mp4-bd-test]` and `MP4 test:`. They
identify the native 204 trigger, verified BL/EL indices, P7 -> P8.1 output,
pair alignment and bounded failure reasons without exposing stream URLs.

## Validation and TV testing

Real FFmpeg + libdovi tests cover synthetic MEL/FEL MP4, reordered B-frame PTS,
both track orders, complete and 23-byte/in-band HEVC configuration, repeated
seeking and audio switching with Original/AAC/AC3 output, bad/missing RPU,
timestamp mismatch and cancellation. They also re-run the existing real MKV
and single-track P8 MP4 regression fixture with the feature compiled in.
ASAN/UBSAN, actual webOS callback routing, settings default/scope/persistence,
normal HTTP buffering and QR redaction/HTTP tests are included in CI.

On TV compare the same failing 117 GB seleZen MP4 with the new option Off and
On, reopening the movie each time. Prefer a 128/256 MiB setting for a heavy
remux. Check actual Dolby Vision activation, sound, seeking and audio changes.
Also verify known-good MKV and native P5/P8/HDR10 MP4 with the option On.
The synthetic fixture tests validate routing and packet integrity; they do
not establish playback or picture quality on a physical LG TV. The separate
46.91 GB P8 release is not assumed to have the same dual-track P7 problem.
