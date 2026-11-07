/* AUTOMATIC SYNC OF ANOTHER VERSION'S AUDIO (owner, 07/11: "Match
 * automatically", on click only). Pure part: no FFmpeg, no threads.
 *
 * A dub or a voice-over keeps the original's music and effects; only the
 * speech differs. Both audios become an energy envelope (one value per 10 ms),
 * then an onset curve (rises of the log energy, which hits, cuts and music
 * share), and the offset is the lag where the two curves correlate best,
 * searched around the current offset. The result is only taken when its peak
 * is clearly above the next best one: otherwise the sync stays manual. */
#ifndef NV_DTS_MATCH_H
#define NV_DTS_MATCH_H
#define DTS_MATCH_HZ 100        /* envelope values per second */
#define DTS_MATCH_SECONDS 30    /* captured from each audio */
#define DTS_MATCH_MAX (DTS_MATCH_HZ * DTS_MATCH_SECONDS)

typedef struct {
  double offset_s;   /* other version's timestamps + offset = this file's (the app's offset) */
  double score;      /* correlation at the peak, -1..1 */
  double margin;     /* peak minus the best value outside +-250 ms */
  int confident;     /* 1: apply it */
} DtsMatchResult;

/* dts_match.inc (static, included where used):
 * int dts_match_offset(main_env, nm, t0m, ext_env, nd, t0d, center_s, radius_s, out)
 * main_env / ext_env: energies per 10 ms, block 0 at t0 seconds (each in its
 * own file's timeline). Searches center_s +- radius_s. 0 when it could not
 * compare (too little audio, or no overlap), 1 with a result (confident or not). */
#endif
