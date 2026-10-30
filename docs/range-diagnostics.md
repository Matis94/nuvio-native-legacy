# HTTP buffer diagnostics in the webOS test build

This build records `[range-diag] v=1` events when compiled with the existing
`NV_HTTP_BUFFER_DIAG` flag. Collection uses ordinary application logging;
Diagnostics QR only controls exporting the log. The 64/128/256 MiB budgets and
number of HTTP lanes remain unchanged.

SDR/DTS now reuses the final media address after a successful redirect, as the
DV range reader already did. If that address fails (except rate limiting), the
reader resolves the original source again. Cross-origin credential stripping
and TLS verification remain active. A 429 keeps the cached media address and
completed payloads, and shares a pause between the range lanes using that route
until Retry-After expires. Resolver and cached-media traffic have separate
deadlines: a working CDN address can still be used during an API pause.
Missing, invalid or zero delays use an increasing fallback
of 2/4/8/16/30 seconds. Waiting is cancellable by seek, stop or session close.
Existing retry-attempt limits are retained; persistent failures can still fail
playback. A healthy in-flight response cannot clear another lane's active pause.

`[range-route] cached=1 sdr=1` confirms reuse is enabled for an SDR reader.
`[range-cooldown]` records the shared pause in milliseconds. These events never
contain media URLs. The counters add no media buffers.

To compare providers, play the same file through Torrentio for about a minute,
then through AIOStreams for about a minute. Note any buffer drop or playback
stall and export the log after both attempts. Enable Diagnostics QR only when
needed to export it. Stop the test sooner if playback fails.

Each request has an `id`; its redirect hops share that ID. `origHost` is the
host initially requested, `host` is the host that answered this hop, and
`nextHost` is a redirect destination, if present. These fields contain DNS
names only: URL credentials, paths, query strings, fragments, ports and numeric
addresses are excluded. Long DNS names survive QR export only in these exact
diagnostic fields. Raw headers and response bodies are never logged.

Useful fields:

- `kind=chunk` is a bounded byte-range request; `kind=stream` is the existing
  sequential reader. `start`/`end` identify the requested interval (`end=-1`
  means open-ended), and `bytes` counts accepted payload bytes.
- `http` and `curl` identify HTTP and transport results. For 429, check `host`
  to locate the layer refusing the request, rather than blaming the addon
  based only on the selected source's name.
- `retrySeen=1` means the response had Retry-After. `retrySec` is the existing
  parser's value in seconds (numeric delay or HTTP date), capped at INT_MAX.
  Zero can mean zero delay, a past date or an invalid value; -1 means absent.
  Only HTTP 429 activates the shared pause; a Retry-After on a successful
  redirect is not applied to the subsequent media response.
- `hopMs` and `elapsedMs` use a monotonic clock. DNS, TCP, TLS and first-byte
  timings are cumulative curl timings in milliseconds, not separate durations.
- `activeAtStart` counts these readers' active HTTP transfers across workers
  when this hop began. It does not count all app requests or TCP connections.
  `newConn` is curl's count of new connections for this transfer (-1 unknown).
- `stripped` records whether cross-origin/downgrade rules removed caller
  headers. `server` is a fixed product classification, never raw server text.

Successful requests are sampled for the first eight requests and then about
once every ten seconds per worker. Errors and transfers lasting at least three
seconds are recorded independently of sampling; cancelled requests are omitted.
There are no extra network requests, payload copies or per-request heap
allocations for this diagnostic. Small fixed counters and stack buffers are
used, with the existing bounded log export.

The DV range recovery experiment also reports `[range-gap]` when the interval
the demuxer currently needs is incomplete while at least two later intervals
are ready. Only that awaited interval gets an earlier three-second retry
budget while below 75% completion. A nearly complete body retains five seconds;
startup, paused/speculative reads and the final retry retain their existing
limits. `[range-budget]` includes the selected `budgetMs`. Completed later
payloads remain cached; lane count and memory budgets do not change. This
handles one slow CDN response and is independent of HTTP 429 cooldowns.
