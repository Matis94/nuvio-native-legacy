#ifndef NV_AUDIOEXTLISTA_H
#define NV_AUDIOEXTLISTA_H
// "FROM ANOTHER VERSION" IN THE AUDIO MENU (audioext.h): which other sources
// of the same title can lend their audio, and in what order.
//
// Pure: the caller turns each source of the list into an AudioExtEntrada
// (what the add-on declared: languages, audio codec, channels, cache, size)
// and gets back the indexes to show. Nothing here reads the network or the
// files; the real tracks are only known when the converter opens the file.
//
// Rules (owner, 06/11/2026):
//  - only sources with a direct link, not HLS, not a warning page, not the
//    one playing, with at least one declared audio language;
//  - only audio the TV takes as it is (Dolby Digital Plus, Dolby Digital, AAC)
//    or not declared; DTS/TrueHD/Opus/FLAC wait for the conversion phase;
//  - grouped by language: the preferred audio language first, then the
//    second one, then the others alphabetically by code;
//  - inside a language: in the debrid cache first, then unknown, then not
//    cached (or hidden); DD+ before DD before AAC before undeclared; smaller
//    files first (size unknown last);
//  - at most AUDIOEXT_POR_IDIOMA per language and `max` in total.
#include <stdint.h>

enum { AEXT_DDP = 0, AEXT_DD, AEXT_AAC, AEXT_DESCONHECIDO, AEXT_DTS, AEXT_TRUEHD, AEXT_OUTRO };
#define AUDIOEXT_IDIOMAS 6
#define AUDIOEXT_POR_IDIOMA 3

typedef struct AudioExtEntradaS {
  int temUrl;                       // direct link (not a torrent hash only)
  int hls, naoVideo, atual;         // excluded
  char idiomas[AUDIOEXT_IDIOMAS][8];// declared audio languages, ISO codes
  int nIdiomas;
  int codec;                        // AEXT_*
  int canais;                       // 2, 6, 8; 0 = unknown
  int cache;                        // 1 cached, 0 unknown, -1 not cached
  uint64_t bytes;                   // 0 = unknown
} AudioExtEntrada;

typedef struct {
  int indice;                       // into the caller's list
  char idioma[8];                   // the language it is listed under
  int grupo;                        // 0 preferred, 1 second, 2 other
} AudioExtCand;

// `casa(code, pref)` compares languages (the app passes ling_casa); NULL
// compares case-insensitively. Empty prefs are skipped. Returns the count.
int audioext_candidatos(const AudioExtEntrada *e, int n, const char *pref1, const char *pref2,
                        int ocultarForaCache, int (*casa)(const char *, const char *),
                        AudioExtCand *out, int max);

// "DD+ 5.1", "DD 2.0", "AAC 2.0"; "" for an undeclared codec.
void audioext_codec_texto(int codec, int canais, char *out, unsigned tam);
// "6.1 GB", "850 MB"; "" when unknown. The decimal separator is the caller's.
void audioext_tamanho_texto(uint64_t bytes, char *out, unsigned tam);
#endif
