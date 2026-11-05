#ifndef NV_DTS_AUDIO_CODEC_H
#define NV_DTS_AUDIO_CODEC_H
#include <string.h>
/* This experiment is enabled only in the dedicated native-DTS test build.
 * DTS Off is the user opt-in; normal AAC/AC3 routing remains unchanged. */
static inline int nv_dts_native_dv_test(int dv,int original_dts) {
#ifdef NV_DV_NATIVE_DTS_TEST
  return dv && original_dts;
#else
  (void)dv;(void)original_dts;return 0;
#endif
}

/* Experimental DV audio conversion, kept out of normal release routing. */
static inline int nv_dts_truehd_codec(const char *codec) {
#ifdef NV_DV_TRUEHD_TEST
  return !strcmp(codec,"truehd") || !strcmp(codec,"A_TRUEHD");
#else
  (void)codec;return 0;
#endif
}
static inline int nv_dts_audio_feeds(const char *codec,int dv) {
  return !strcmp(codec,"dts") || (dv && (!strcmp(codec,"ac3") ||
         !strcmp(codec,"eac3") || nv_dts_truehd_codec(codec)));
}
#endif
