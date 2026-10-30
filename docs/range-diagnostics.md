# HTTP buffer diagnostics in the webOS test build

This build records `[range-diag] v=1` events when compiled with the existing
`NV_HTTP_BUFFER_DIAG` flag. Collection uses ordinary application logging;
Diagnostics QR only controls exporting the log. No playback, buffer-size,
retry-delay, connection-count or redirect policy is changed.

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
  This is observation only, not a change to the retry scheduler.
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
