# Custom 2.0.7 playback indicators — test 2

The numeric download indicator reports measured payload and total capacity.
The official DTS/DV queue adds one active range (approximately 4 MiB): a
256 MiB queue therefore reports 260 MiB total, and a 128 MiB queue reports
132 MiB. The existing free-memory clamp can lower a requested 256 MiB queue.
Allocation policy is unchanged. The label now says `total` and shows the
selected setting when it differs from the measured capacity.

Buffering timestamps and native buffer edges are cleared when retiring a
pipeline and when adopting a new converter revision. An explicit `playing`
event also ends buffering if firmware omitted `bufferingEnd`; an explicit
`bufferingStart` in the same payload takes precedence. Current-time updates
alone do not suppress buffering, and fresh underruns remain visible.

Host regressions cover missing bufferingEnd, native-to-DV handoff, converter
reloads before playing, fresh buffering, ordinary native capacity, total
260/132/68 MiB and selected settings. The spinner and teardown regressions
fail against the preceding custom-2.0.7 source.

The test IPK keeps the app ID/version and all existing custom features. The
build strips static symbols/debug sections before packaging; app logs remain.
Validate on LG with P7 playback, pause/resume, seeking and a real buffering
event. No Samsung/Android playback behavior or buffer allocation is changed.
