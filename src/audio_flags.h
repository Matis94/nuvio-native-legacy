#ifndef NV_AUDIO_FLAGS_H
#define NV_AUDIO_FLAGS_H
#include "streams.h"
#define AUDIO_FLAGS_MAX 48
typedef struct {
  unsigned n, outros;
  char pais[AUDIO_FLAGS_MAX][3];
} AudioFlags;
// Declared audio only. Country symbols follow AIOStreams conventions; never
// infer tracks/languages from Dual/Multi, subtitle languages or filenames.
void audio_flags_ler(const Stream *s, AudioFlags *saida);
#endif
