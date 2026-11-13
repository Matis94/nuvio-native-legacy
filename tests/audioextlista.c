// "From another source" candidates: filters, language grouping, order, copies, limits.
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

static AudioExtOpcoes opc(const char *p1, const char *p2, int modo) {
  AudioExtOpcoes o;
  memset(&o, 0, sizeof o);
  o.pref1 = p1; o.pref2 = p2; o.semDts = 1; o.ocultarCopias = 1; o.modo = modo; o.casa = casa;
  return o;
}

int main(void) {
  static AudioExtEntrada e[64];
  static AudioExtCand c[64];
  AudioExtOpcoes o;
  int n = 0, k;
  e[n++] = fonte("en", AEXT_DDP, 6, 1, 8);            // 0 English
  e[n++] = fonte("pl", AEXT_DD, 6, 1, 3);             // 1 Polish DD, cached, small
  e[n++] = fonte("pol", AEXT_DDP, 6, 1, 6);           // 2 Polish DD+, cached
  e[n++] = fonte("pl", AEXT_DDP, 6, -1, 2);           // 3 Polish DD+, not cached
  e[n++] = fonte("pl", AEXT_DTS, 6, 1, 4);            // 4 Polish DTS: converted (hidden with DTS off)
  e[n++] = fonte("pl", AEXT_DDP, 6, 0, 0);            // 5 Polish DD+, cache and size unknown
  e[n++] = fonte("cs,en", AEXT_DDP, 6, 1, 5);         // 6 Czech + English
  e[n++] = fonte("de", AEXT_AAC, 2, 1, 1);            // 7 German AAC
  e[n++] = fonte("", AEXT_DDP, 6, 1, 1);              // 8 no declared language
  e[n++] = fonte("pl", AEXT_DDP, 6, 1, 1); e[n - 1].hls = 1;      // 9 HLS
  e[n++] = fonte("pl", AEXT_DDP, 6, 1, 1); e[n - 1].atual = 1;    // 10 playing now
  e[n++] = fonte("pl", AEXT_DDP, 6, 1, 1); e[n - 1].temUrl = 0;   // 11 torrent only
  e[n++] = fonte("pl", AEXT_DESCONHECIDO, 0, 1, 7);  // 12 Polish, codec and channels not declared
  e[n++] = fonte("pl", AEXT_DDP, 2, 1, 0.5);          // 13 Polish DD+ stereo, cached, smallest

  o = opc("pl", "en", AEXT_MODO_BEST);
  k = audioext_lista(e, n, &o, c, 64);
  // Polish first: cached 5.1 (DD+ 6 GB, DD 3 GB), cached channels unknown
  // (12), cached stereo (13, smaller but stereo), then cache unknown, then not cached.
  assert(c[0].indice == 2 && c[0].grupo == AEXT_G_PREF1);
  assert(c[1].indice == 1);
  assert(c[2].indice == 12);
  assert(c[3].indice == 13);
  assert(c[4].indice == 5 && c[5].indice == 3);
  // Then English (second language): the Czech one that also declares English
  // is smaller, so first.
  assert(c[6].grupo == AEXT_G_PREF2 && c[6].indice == 6 && c[7].indice == 0);
  // Then the version with no language (3rd, "likely original"), then German.
  assert(c[8].grupo == AEXT_G_SEM && c[8].indice == 8 && !c[8].idioma[0]);
  assert(c[9].grupo == AEXT_G_OUTRA && !strcmp(c[9].idioma, "de") && c[9].indice == 7);
  assert(k == 10);
  for (int i = 0; i < k; i++)
    assert(c[i].indice != 4 && c[i].indice != 9 && c[i].indice != 10 && c[i].indice != 11);
  // Hide not cached.
  o.ocultarForaCache = 1;
  k = audioext_lista(e, n, &o, c, 64);
  for (int i = 0; i < k; i++) assert(c[i].indice != 3);
  o.ocultarForaCache = 0;
  // DTS conversion on: the Polish DTS joins, after DD+/DD, inside the cached 5.1.
  o.semDts = 0;
  k = audioext_lista(e, n, &o, c, 64);
  assert(c[0].indice == 2 && c[1].indice == 1 && c[2].indice == 4);
  o.semDts = 1;
  // No preference: the one without a language still comes before the others.
  o = opc("", "", AEXT_MODO_BEST);
  k = audioext_lista(e, n, &o, c, 64);
  assert(c[0].grupo == AEXT_G_SEM && c[0].indice == 8);
  assert(!strcmp(c[1].idioma, "cs") && !strcmp(c[2].idioma, "de"));
  // Total limit.
  o = opc("pl", "en", AEXT_MODO_BEST);
  assert(audioext_lista(e, n, &o, c, 2) == 2);

  // COPIES OF ONE FILE: the same torrent file through two add-ons, and the
  // same size/codec/channels/languages without a hash. The cached one stays,
  // the others become its fallbacks, best first; off: all shown.
  n = 0;
  e[n++] = fonte("pl", AEXT_DDP, 6, 0, 4.2); snprintf(e[n - 1].ident, sizeof e[0].ident, "t:abc:1");  // 0
  e[n++] = fonte("pl", AEXT_DDP, 6, 1, 4.2); snprintf(e[n - 1].ident, sizeof e[0].ident, "t:abc:1");  // 1 cached copy
  e[n++] = fonte("pl", AEXT_DDP, 6, 1, 4.2); snprintf(e[n - 1].ident, sizeof e[0].ident, "t:abc:2");  // 2 another file, same size
  e[n++] = fonte("pl", AEXT_DD, 6, 1, 3.1);                                                         // 3
  e[n++] = fonte("pol", AEXT_DD, 6, -1, 3.1);                                                       // 4 copy of 3 (no hash)
  e[n++] = fonte("pl", AEXT_DD, 2, 1, 3.1);                                                         // 5 stereo: not a copy
  e[n++] = fonte("pl", AEXT_DD, 6, 1, 3.1); snprintf(e[n - 1].ident, sizeof e[0].ident, "v:ffff");  // 6 hash vs none: by size
  o = opc("pl", "en", AEXT_MODO_TODOS);
  k = audioext_lista(e, n, &o, c, 64);
  assert(k == 4);
  assert(c[0].indice == 1 && c[0].nReservas == 1 && c[0].reservas[0] == 0);
  assert(c[1].indice == 2 && c[1].nReservas == 0);
  assert(c[2].indice == 3 && c[2].nReservas == 2 && c[2].reservas[0] == 6 && c[2].reservas[1] == 4);
  assert(c[3].indice == 5);
  assert(audioext_mesmo_arquivo(&e[0], &e[1]) && !audioext_mesmo_arquivo(&e[1], &e[2]));
  o.ocultarCopias = 0;
  assert(audioext_lista(e, n, &o, c, 64) == n);

  // LIMITS: Best keeps 10 of each preferred language, 5 without a language
  // and 10 of each other language; All keeps everything (up to max).
  n = 0;
  for (int i = 0; i < 12; i++) e[n++] = fonte("pl", AEXT_DDP, 6, 1, 1 + i);
  for (int i = 0; i < 7; i++) e[n++] = fonte("", AEXT_DDP, 6, 1, 1 + i);
  for (int i = 0; i < 11; i++) e[n++] = fonte("fr", AEXT_DDP, 6, 1, 1 + i);
  for (int i = 0; i < 3; i++) e[n++] = fonte("de", AEXT_DDP, 6, 1, 1 + i);
  o = opc("pl", "en", AEXT_MODO_BEST);
  k = audioext_lista(e, n, &o, c, 64);
  { int g[4] = { 0 }, de = 0, fr = 0;
    for (int i = 0; i < k; i++) { g[c[i].grupo]++; if (!strcmp(c[i].idioma, "de")) de++; if (!strcmp(c[i].idioma, "fr")) fr++; }
    assert(g[AEXT_G_PREF1] == AUDIOEXT_BEST_PREF && g[AEXT_G_SEM] == AUDIOEXT_BEST_DESC);
    assert(de == 3 && fr == AUDIOEXT_BEST_OUTRA);
    // Other languages alphabetically: German before French? "de" < "fr".
    for (int i = 1; i < k; i++) if (c[i].grupo == AEXT_G_OUTRA && c[i - 1].grupo == AEXT_G_OUTRA)
      assert(strcmp(c[i - 1].idioma, c[i].idioma) <= 0); }
  o.modo = AEXT_MODO_TODOS;
  assert(audioext_lista(e, n, &o, c, 64) == n);
  assert(audioext_lista(e, n, &o, c, 20) == 20);

  assert(audioext_converte(AEXT_DTS) && audioext_converte(AEXT_TRUEHD) && !audioext_converte(AEXT_DDP));
  char t[32];
  audioext_codec_texto(AEXT_DDP, 6, t, sizeof t); assert(!strcmp(t, "DD+ 5.1"));
  audioext_codec_texto(AEXT_AAC, 2, t, sizeof t); assert(!strcmp(t, "AAC 2.0"));
  audioext_codec_texto(AEXT_DESCONHECIDO, 6, t, sizeof t); assert(!t[0]);
  audioext_codec_texto(AEXT_TRUEHD, 8, t, sizeof t); assert(!strcmp(t, "TrueHD 7.1"));
  audioext_tamanho_texto(6100000000ULL, t, sizeof t); assert(!strcmp(t, "6.1 GB"));
  audioext_tamanho_texto(850000000ULL, t, sizeof t); assert(!strcmp(t, "850 MB"));
  audioext_tamanho_texto(0, t, sizeof t); assert(!t[0]);
  puts("audioextlista: filters, language groups (unknown 3rd), cache/channels/codec/size order, copies and Best/All limits passed");
  return 0;
}
