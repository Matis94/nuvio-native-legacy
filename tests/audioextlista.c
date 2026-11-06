// "From another version" candidates: filters, language grouping, order, limits.
#include "../src/audioextlista.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

static AudioExtEntrada fonte(const char *langs, int codec, int canais, int cache, double gb) {
  AudioExtEntrada e;
  memset(&e, 0, sizeof e);
  e.temUrl = 1; e.codec = codec; e.canais = canais; e.cache = cache;
  e.bytes = (uint64_t)(gb * 1e9);
  char tmp[64]; snprintf(tmp, sizeof tmp, "%s", langs);
  for (char *t = strtok(tmp, ","); t && e.nIdiomas < AUDIOEXT_IDIOMAS; t = strtok(NULL, ","))
    snprintf(e.idiomas[e.nIdiomas++], sizeof e.idiomas[0], "%s", t);
  return e;
}
// "pl" also matches "pol", like ling_casa.
static int casa(const char *code, const char *pref) {
  if (!strcasecmp(code, pref)) return 1;
  return (!strcasecmp(pref, "pl") && !strcasecmp(code, "pol")) ||
         (!strcasecmp(pref, "en") && !strcasecmp(code, "eng"));
}

int main(void) {
  AudioExtEntrada e[16];
  AudioExtCand c[16];
  int n = 0, k;
  e[n++] = fonte("en", AEXT_DDP, 6, 1, 8);            // 0 English
  e[n++] = fonte("pl", AEXT_DD, 6, 1, 3);             // 1 Polish DD, cached, small
  e[n++] = fonte("pol", AEXT_DDP, 6, 1, 6);           // 2 Polish DD+, cached
  e[n++] = fonte("pl", AEXT_DDP, 6, -1, 2);           // 3 Polish DD+, not cached
  e[n++] = fonte("pl", AEXT_DTS, 6, 1, 4);            // 4 Polish DTS: waits for conversion
  e[n++] = fonte("pl", AEXT_DDP, 6, 0, 0);            // 5 Polish DD+, cache and size unknown
  e[n++] = fonte("cs,en", AEXT_DDP, 6, 1, 5);         // 6 Czech + English
  e[n++] = fonte("de", AEXT_AAC, 2, 1, 1);            // 7 German AAC
  e[n++] = fonte("", AEXT_DDP, 6, 1, 1);              // 8 no declared language
  e[n++] = fonte("pl", AEXT_DDP, 6, 1, 1); e[n - 1].hls = 1;      // 9 HLS
  e[n++] = fonte("pl", AEXT_DDP, 6, 1, 1); e[n - 1].atual = 1;    // 10 playing now
  e[n++] = fonte("pl", AEXT_DDP, 6, 1, 1); e[n - 1].temUrl = 0;   // 11 torrent only
  e[n++] = fonte("pl", AEXT_DESCONHECIDO, 0, 1, 7);  // 12 Polish, codec not declared

  k = audioext_candidatos(e, n, "pl", "en", 0, casa, c, 16);
  // Polish first (3 at most): cached DD+ 6 GB, cached DD 3 GB, cached undeclared 7 GB.
  assert(k >= 3);
  assert(c[0].indice == 2 && c[0].grupo == 0);
  assert(c[1].indice == 1);
  assert(c[2].indice == 12);
  // Then English (second language): the English source, and the Czech one
  // that also declares English, smaller first.
  assert(c[3].grupo == 1 && c[3].indice == 6 && c[4].indice == 0);
  // Then the rest alphabetically: German.
  assert(c[5].grupo == 2 && !strcmp(c[5].idioma, "de") && c[5].indice == 7);
  assert(k == 6);
  for (int i = 0; i < k; i++)
    assert(c[i].indice != 4 && c[i].indice != 8 && c[i].indice != 9 && c[i].indice != 10 && c[i].indice != 11);

  // Bucket of 3: drop the cached ones and the unknown/uncached take their place.
  e[1].temUrl = e[2].temUrl = e[12].temUrl = 0;
  k = audioext_candidatos(e, n, "pl", "en", 0, casa, c, 16);
  assert(c[0].indice == 5 && c[1].indice == 3);                 // unknown cache before not cached
  k = audioext_candidatos(e, n, "pl", "en", 1, casa, c, 16);   // hide not cached
  assert(c[0].indice == 5 && c[1].grupo == 1);

  // No preference: everything groups by code, alphabetically.
  e[1].temUrl = e[2].temUrl = e[12].temUrl = 1;
  k = audioext_candidatos(e, n, "", "", 0, NULL, c, 16);
  assert(!strcmp(c[0].idioma, "cs") && !strcmp(c[1].idioma, "de"));
  // Total limit.
  assert(audioext_candidatos(e, n, "pl", "en", 0, casa, c, 2) == 2);

  char t[32];
  audioext_codec_texto(AEXT_DDP, 6, t, sizeof t); assert(!strcmp(t, "DD+ 5.1"));
  audioext_codec_texto(AEXT_AAC, 2, t, sizeof t); assert(!strcmp(t, "AAC 2.0"));
  audioext_codec_texto(AEXT_DESCONHECIDO, 6, t, sizeof t); assert(!t[0]);
  audioext_tamanho_texto(6100000000ULL, t, sizeof t); assert(!strcmp(t, "6.1 GB"));
  audioext_tamanho_texto(850000000ULL, t, sizeof t); assert(!strcmp(t, "850 MB"));
  audioext_tamanho_texto(0, t, sizeof t); assert(!t[0]);
  puts("audioextlista: filters, language groups, cache/codec/size order and limits passed");
  return 0;
}
