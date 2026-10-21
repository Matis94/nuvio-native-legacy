# Optional buffer RAM reduction — LG webOS

Settings > Playback > Buffer size now has **Automatically reduce buffer when RAM is low** directly below it. Default: **On** (the existing official policy).

- On: step down 256/128/64 MiB as needed to preserve the 100 MiB free-memory reserve, accounting for the converter's additional active range.
- Off: request the selected 64/128/256 MiB even with low or unknown free RAM. The app may fail playback or be closed if memory is exhausted.
- Auto remains the backend default. No size above 256 MiB can be requested. Allocation failure handling and network validation/retry limits remain unchanged.

The switch applies on the next video open, including ordinary HTTP movie caching and DTS/DV conversion. It does not resize the running session. It is webOS-only, stored locally on the device, and excluded from account sync and device backup. Other platforms retain the existing RAM guard.

The diagnostic overlay reports total capacity, which may exceed the selected read-ahead amount by the active range (about 4 MiB). `255 / 260 MiB total (256 MiB selected)` means approximately 255 MiB retained out of 260 MiB total capacity; no RAM reduction occurred in that example.

Host coverage: desktop/Android/Tizen keep the RAM guard enabled and hide the switch; webOS exposes it. Existing memory-boundary tests plus low/unknown-memory opt-out, Auto/invalid values, append-only settings IDs, default On, save/reload, local-only sync protection, and placement directly below Buffer size. Physical LG verification remains pending.

## Upstream buffer policy update

Adopts upstream `da7b893fc51388fceb676015f0a67a525b37bcca`: the reserve is now **100 MiB**, down from 192 MiB. With the guard enabled, 256 MiB needs at least 360 MiB available, 128 MiB needs 232 MiB, and 64 MiB needs 168 MiB (including the 4 MiB active range). This is checked when opening/preloading a video, not a running-session memory monitor. The local switch above bypasses this same official clamp when Off.

The official performance overlay also shows available system memory during playback. This supplements the existing optional Green-button RAM overlay.

## Independent DTS playback recovery

**DTS playback recovery** is a separate device-local setting, **On** by default. It controls failed HTTP range retries, early retry of an actively awaited stalled range, bounded short forward audio timestamp padding and bounded automatic native playback recovery. It works with ordinary SDR/DTS and DV conversion regardless of the P7 preference. It applies to the next video and is excluded from account sync and device backup.

**Dolby Vision P7 to P8.1** controls only P7 conversion. Turning recovery Off does not disable P7 conversion, core fallback, or explicit seeking/source reuse. The full custom build retains its existing default P7 preference, ordinary HTTP movie cache, filename wrapping, diagnostic export, and RAM overlay.

Host checks cover the independent On/Off combinations, persistence and sync isolation, exact HTTP retry counts, timestamp gaps, controller recovery and manual seeks, P7 MEL/FEL conversion with recovery On/Off, the 100 MiB boundaries and the RAM opt-out. Physical LG verification is still required.
