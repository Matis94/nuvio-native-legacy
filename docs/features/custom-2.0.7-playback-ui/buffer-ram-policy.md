# Optional buffer RAM reduction — LG webOS

Settings > Playback > Buffer size now has **Automatically reduce buffer when RAM is low** directly below it. Default: **On** (the existing official policy).

- On: step down 256/128/64 MiB as needed to preserve the 192 MiB free-memory reserve, accounting for the converter's additional active range.
- Off: request the selected 64/128/256 MiB even with low or unknown free RAM. The app may fail playback or be closed if memory is exhausted.
- Auto remains the backend default. No size above 256 MiB can be requested. Allocation failure handling and network validation/retry limits remain unchanged.

The switch applies on the next video open, including ordinary HTTP movie caching and DTS/DV conversion. It does not resize the running session. It is webOS-only, stored locally on the device, and excluded from account sync and device backup. Other platforms retain the existing RAM guard.

The diagnostic overlay reports total capacity, which may exceed the selected read-ahead amount by the active range (about 4 MiB). `255 / 260 MiB total (256 MiB selected)` means approximately 255 MiB retained out of 260 MiB total capacity; no RAM reduction occurred in that example.

Host coverage: desktop/Android/Tizen keep the RAM guard enabled and hide the switch; webOS exposes it. Existing memory-boundary tests plus low/unknown-memory opt-out, Auto/invalid values, append-only settings IDs, default On, save/reload, local-only sync protection, and placement directly below Buffer size. Physical LG verification remains pending.
