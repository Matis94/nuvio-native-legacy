# From addon: rich source cards

The From addon source-text option uses the supplied name and description in
full-width cards, with a separate badge row, colored emoji and country flags.
Five description lines are visible without focus. The selected card expands
to at most sixteen wrapped lines, from up to twelve source lines. Long release
filenames wrap at punctuation or complete UTF-8 boundaries. A filename supplied
separately by the addon is included when it is absent from the description.

The From Nuvio layout and playback/source-selection rules retain their existing
behavior. Audio hints remain presentation/filter evidence, not verified tracks.
Current badges, addon tabs, language filters, ordering and the source-details
action continue to use the existing implementations.

## Reference

Visual reference: the owner's Oppenheimer source-card screenshot, supplied on
2026-10-30. Reviewed NuvioMedia/NuvioTVSmart at
`53b4592016272449d2c930fdc8276b42f020efc1`, in particular:

- `js/ui/screens/stream/streamVirtualizer.js`: prefix offsets and binary lookup.
- `js/ui/screens/stream/streamScreenMethods-09-render.js`: bounded visible rows,
  stable identity during arrivals, and lazy badge hydration.
- `js/ui/screens/stream/streamScreenMethods-08-build-web-os-native-player-launch-parameters.js`:
  badge placement, formatter heading and description lines.
- `js/core/streams/streamDisplayText.js`: styled-character normalization.

This is a native C adaptation using the existing SDL/OpenGL source sheet; it
does not introduce a browser or copy the web application's JavaScript into it.

## Work on large lists

- Keep the existing prefix positions and binary search for visible cards.
- Closed offscreen geometry only counts nonempty source lines. It does not
  clean text, measure fonts or prepare textures for all previous indices.
- Parse and measure a card only when it is displayed or expanded.
- Cache at most 32 parsed cards, indexed by stable raw source index. The cache
  survives appends and is cleared on list replacement, including same-size lists.
- Cache layout by width, text size/weight and font family. Focus changes reuse
  the same neutral text textures with tint/alpha instead of different RGB text.
- Decode the 48px local emoji through the existing asynchronous texture LRU.
  Missing/pending artwork uses an existing monochrome icon. Nothing is fetched
  from a CDN during source navigation.

The one-time source/filter/order rebuild still depends on the number of
sources. No TV frame-rate claim is made before device testing.

## Artwork

82 local emoji/flag images derive from Twemoji v16.0.1, by Twitter, Inc. and
other contributors, under CC BY 4.0. Attribution, the complete license and a
source/checksum manifest ship in `deploy/app/art/icones/emoji-*`.
`tools/formatter_emoji_assets.py` regenerates the small asset set and C lookup
table; it is a developer tool, never a runtime download path.

## Validation and later TV testing

No application/package compilation or C test execution was performed at the
owner's request. Static checks cover new C syntax, shell syntax, translation
tables, PNG integrity/transparency, manifest checksums and fallback availability.

`tests/fontes_formatter.sh` is prepared for an explicitly authorized future test
run: 2000-source direct jumps, fixed cache bounds, eviction, append/replacement,
UTF-8 wrapping, emoji/flags, filter changes, and prefix geometry. Existing
`tests/fontes_quadro.sh` covers bounded per-frame source-row visits.

On the TV, select From addon and test a 1000–2000-source result list. Hold Down,
reverse direction, switch addon/language/cache filters, and allow another addon
to finish while a card is selected. Verify that selection remains on the same
source, badges stay in their own row, flags and DUAL/MULTI survive, and a long
filename expands without overlapping the next card. Compare From Nuvio and
confirm that its layout is unchanged. Use source details for descriptions beyond
the card's bounded expansion.
