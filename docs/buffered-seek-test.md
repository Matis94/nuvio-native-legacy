# Buffered seeking and native pipeline reuse experiment

Base: upstream `0ce56918b116dba810b642bec2c3ac8a9ef2adb0` (2026-10-23).

- Keep validated speculative HTTP ranges when FFmpeg seeks into their payload,
  including offsets inside a range. Transfer the existing allocation into the
  active cache instead of downloading the range again.
- Preserve the native HTTP ring's history until the writer needs its storage.
  Ring capacity, range-slot count and official RAM safety policy are unchanged.
- Switch supported audio on the existing demuxer and byte cache. Free the old
  audio codec chain before creating the next one; preserve video/DV conversion
  state and reset codec timestamps through a normal container seek.
- Provide an explicit, default-off `dts_playback_next_seamless` experiment.
  Healthy sessions can flush/re-preroll their current native pipeline for a
  seek or compatible audio switch, avoiding destroy/create/Load. A changed
  codec, channel count or sample rate still requires a fresh native Load.
- Suppress old-segment clocks during reconfiguration. If the renderer does not
  advance after two seconds of active playback, use the proven fresh-Load
  recovery path and disable seamless attempts for that session. Paused time
  does not consume that deadline. Existing bounded recovery remains in place.

The diagnostic IPK opts in to native reuse and includes temporary RAM and
buffer counters. These are TV experiments, not a guarantee that each webOS
firmware supports a continuous decoder timeline. No decoded-video cache or
second native player is introduced. The memory limits apply to buffered media;
codec and system allocations can still vary during a track change.

Validation:

- `tests/dts_cached_seek.sh`: real HTTP, all three budgets, exact bytes and
  pointer ownership across interior cached seeks, unchanged payload ceiling.
- `tests/moviebuffer.sh`: cached native HTTP replacements do not restart the
  origin connection or enlarge the ring; existing range/fallback tests.
- `tests/dts_buffer_reporting.sh`: real DTS tracks at all three budgets,
  P7 disabled, audio switching without container reopen and invalid selection.
- `tests/dts_playback.sh`: original controller suite plus in-place seek/audio,
  paused seek, DV session, refusal/format-change fallback and the captured
  old-segment/BufferFull failure with bounded recovery.

TV checks: native HTTP, SDR/DTS and P7/DTS; forward/backward inside the buffer;
paused seeks; DTS-to-DTS audio switch; and codec-changing audio switch. Watch
RAM current/peak, available RAM, counter capacity and `dtsStage` events.
