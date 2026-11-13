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
// Rules (owner, 06/11/2026, reworked 13/11/2026 for "From another source"):
//  - only sources with a direct link, not HLS, not a warning page, not the
//    one playing;
//  - any audio: Dolby Digital Plus, Dolby Digital and AAC play as they are,
//    DTS, TrueHD, Opus and FLAC are converted (AC-3 5.1 or AAC stereo, like
//    the main file's DTS); DTS only while DTS conversion is not off;
//  - grouped by language: the preferred audio language first, then the
//    second one, then the sources that declare no language (often the
//    original, untagged English of a movie), then the others alphabetically;
//  - inside a language: in the debrid cache first, then unknown, then not
//    cached (or hidden); 5.1 or more, then undeclared channels, then stereo;
//    DD+, DD, AAC, undeclared, then the converted ones (DTS, TrueHD, other);
//    smaller files first (size unknown last); then the list's own order;
//  - copies of the same file (same hash, or same size, codec, channels and
//    languages) show once, the best one; the others stay as its fallbacks;
//  - "Best": at most AUDIOEXT_BEST_PREF of each preferred language,
//    AUDIOEXT_BEST_DESC without a language and AUDIOEXT_BEST_OUTRA of each
//    other language; "All": everything, up to `max`.
#include <stdint.h>

enum { AEXT_DDP = 0, AEXT_DD, AEXT_AAC, AEXT_DESCONHECIDO, AEXT_DTS, AEXT_TRUEHD, AEXT_OUTRO };
#define AUDIOEXT_IDIOMAS 6
#define AUDIOEXT_BEST_PREF 10
#define AUDIOEXT_BEST_DESC 5
#define AUDIOEXT_BEST_OUTRA 10
#define AUDIOEXT_TODOS_MAX 200
#define AUDIOEXT_RESERVAS 3
// Groups, in this order on screen.
enum { AEXT_G_PREF1 = 0, AEXT_G_PREF2, AEXT_G_SEM, AEXT_G_OUTRA };
enum { AEXT_MODO_BEST = 0, AEXT_MODO_TODOS };

typedef struct AudioExtEntradaS {
  int temUrl;                       // direct link (not a torrent hash only)
  int hls, naoVideo, atual;         // excluded
  char idiomas[AUDIOEXT_IDIOMAS][8];// declared audio languages, ISO codes
  int nIdiomas;
  int codec;                        // AEXT_*
  int canais;                       // 2, 6, 8; 0 = unknown
  int cache;                        // 1 cached, 0 unknown, -1 not cached
  uint64_t bytes;                   // 0 = unknown
  char ident[64];                   // the file itself (hash, torrent:file); "" unknown
  int foraFiltro;                   // its add-on switched off in Settings > Addons
} AudioExtEntrada;

typedef struct {
  int indice;                       // into the caller's list
  char idioma[8];                   // the language it is listed under ("" none)
  int grupo;                        // AEXT_G_*
  int nReservas;                    // hidden copies of the same file
  int reservas[AUDIOEXT_RESERVAS];  // their indexes, best first
} AudioExtCand;

typedef struct {
  const char *pref1, *pref2;        // "" skipped
  int ocultarForaCache, semDts;
  int ocultarCopias;                // one row per file (Settings, on by default)
  int modo;                         // AEXT_MODO_*
  // compares languages (the app passes ling_casa); NULL: case-insensitive.
  int (*casa)(const char *, const char *);
} AudioExtOpcoes;

// The candidates in screen order. Returns the count (at most `max`).
int audioext_lista(const AudioExtEntrada *e, int n, const AudioExtOpcoes *o, AudioExtCand *out, int max);
// 1 when `a` and `b` are the same file (the rule above).
int audioext_mesmo_arquivo(const AudioExtEntrada *a, const AudioExtEntrada *b);
// 1 when this codec is converted on the way to the TV (DTS, TrueHD, other).
int audioext_converte(int codec);

// "DD+ 5.1", "DD 2.0", "AAC 2.0", "DTS 5.1", "TrueHD 7.1"; "" undeclared/other.
void audioext_codec_texto(int codec, int canais, char *out, unsigned tam);
// "6.1 GB", "850 MB"; "" when unknown. The decimal separator is the caller's.
void audioext_tamanho_texto(uint64_t bytes, char *out, unsigned tam);
#endif
