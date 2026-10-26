# Experimental MP4 BD REMUX test 3

Base: v17-ui, commit `85ca963f28afb9ddd0edded0e013fca579493fbc`.
Branch: `build/mp4-bd-remux-test`. The `build/playback-v17-no-qr` branch is not modified.

## Scope and settings

Settings > Playback > this TV's sound/video section:
**Dolby Vision MP4 (Experimental)**, default **Off**, stored on this TV only.
Reopen the video after changing it. Old preferences retain their IDs and keys.

Off retains the v17-ui playback policy. On retries a native startup demux error
204 only for a source recognized as MP4, once per opening. A playing source,
MKV, live source and an already attempted converter do not enter this retry.
The worker then verifies the real container metadata: exactly two HEVC video
tracks, one HDR10-compatible base and a separate P7 EL/RPU track. Unrelated
MP4, P5, ambiguous tracks and encryption are rejected. A single-track P8.1
source is also accepted only with version 1.0, compatibility ID 1, BL/RPU
present, EL absent, verified HDR10 base metadata and a valid first P8 RPU.
Its metadata passes through unchanged; the P7 converter is never invoked. A BD REMUX filename
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

Test 2 reproduces the TV's initial configuration rejection using a 23-byte
hvcC and no MP4 colr box. The minimal FFmpeg HEVC parser reads dimensions but
does not publish SPS VUI colour fields. For verified separate-track P7 only,
missing HDR fields are recovered from a bounded base SPS read (hvcC arrays or
in-band packets), without decoding pictures or assuming unknown colour is HDR.
The probe is limited to 64 packets / 4 MiB and an 8 KiB SPS, within the same
16 MiB reservation. An in-band probe rewinds before audio setup/resume so the
opening pictures and audio are retained. Missing SPS, explicit SDR, colour
conflicts and invalid headers still fail. Logs now give individual BL/EL
parameters, specific rejection reasons and native numeric errors even if the
backend JSON has been removed by diagnostic redaction.

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
not establish playback or picture quality on a physical LG TV. Also test the separate 46.91 GB P8 release; recovery requires its actual
compatibility ID to be 1 and valid P8 metadata, rather than guessing from
the filename.

## Test 3 seek/resume corrections

A MOV demuxer can seek the EL track to a much older sync sample than the BL.
Obsolete RPUs then fill the 64-entry queue before its matching base picture
arrives. Reopening at the saved position repeats this failure. The dual-track
reader probes the actual first base keyframe's DTS/PTS, repeats the identical
seek before output, and discards only EL samples strictly older than that
verified anchor. The first BL after replay must match the anchor exactly.
Probe/discard work is limited to 1024 packets / 8 MiB; base packets retain their
4 MiB limit, exact DTS/PTS pairing and all existing queue limits. No video is
decoded and the HTTP read-ahead allocation is unchanged.

For opt-in MP4 sessions only, consecutive seek commands retain the latest
requested target while the first accepted native seek awaits a valid clock.
The existing 4/12-second no-progress deadlines and reload fallback remain in
force. Pause, cancellation and audio selection retain bounded behavior. Other
playback sessions retain the v17-ui controller policy.

The real-engine regression has a deliberately coarse EL sync index and over
64 old RPUs. It covers saved-position startup, repeated forward/back seeks,
audio selection, reopening, all three audio outputs, decoded HEVC, exact RPUs,
monotonic DTS and the unchanged memory capacity. P8 tests compare every RPU
byte and decoded frame hash, reject unsupported metadata, and exercise
64/128/256 MiB presets. The native seek controller has separate rapid-command,
feature-off, timeout, pause, stop and audio tests.

## Buffer comparison with v17-ui

The cacheboost/moviebuffer implementations, pipeline/adapters, HTTP range
transport, source readers, memory guards and refill policy are unchanged.
This MP4 recovery reserves four existing 4 MiB prefetch slots (about 16 MiB)
for bounded metadata work. Conversion diagnostics count prefetch slots plus
one active read block; the reported capacity is therefore approximately:

| Selected preset | v17 conversion path | MP4 recovery |
| --- | --- | --- |
| 64 MiB | 68 MiB | 52 MiB |
| 128 MiB | 132 MiB | 116 MiB |
| 256 MiB | 260 MiB | 244 MiB |

For `240/244 (256 selected)`, 240 is current buffered payload, 244 is capacity,
and 256 is the preset. This is not total application RAM. Regular HTTP RAM
buffering and its safety reductions remain separate and unchanged.

On TV specifically resume the previously failing 117 GB movie near 3835 s
(about 1:04), seek forward/back, close/reopen, then test successive seeks and
audio changes. Export the QR diagnostic log if any step fails.
