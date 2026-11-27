/* AUDIO FROM ANOTHER SOURCE: what the subtitle match (doadorleg.c) reads of
 * the two open files. On LG the converter answers it (dts/doador_webos.c);
 * these are the types and calls of the Samsung donor server's header that
 * the match needs, nothing more.
 *
 * Fonte 0 is the playing file, fonte 1 the other version. The embedded TEXT
 * subtitle tracks are listed as their seek index has them (the start of each
 * indexed block, in that file's own time): no byte more is downloaded.
 * Tracks with fewer than 20 points are left out. */
#ifndef NV_DOADORFONTES_H
#define NV_DOADORFONTES_H
#include <stddef.h>

#define DOA_LEG_MAX 32   /* every text track of a file */
/* n: index points; falas: the lines they make once a burst (an ASS line drawn
 * in pieces, karaoke, a typeset sign: starts within 0.15 s; 0.02 s for the
 * others) counts as one, what the match compares (0 = not counted: n). */
typedef struct { char lingua[16], nome[64], codec[16]; int forcada, n, falas; } DoaLegInfo;
#define DOA_SURTO_ASS 0.15
#define DOA_SURTO_SRT 0.02
static inline int doa_falas(const double *t, int n, int ass) {
  int i, m = 0;
  double ult = -1e9;
  for (i = 0; i < n; i++) {
    if (t[i] < 0) continue;
    if (m && t[i] - ult < (ass ? DOA_SURTO_ASS : DOA_SURTO_SRT)) continue;
    ult = t[i]; m++;
  }
  return m;
}
int  doa_legendas(int fonte, DoaLegInfo *out, int max);
/* Start times (s, sorted) of track `k` of doa_legendas; returns the count. */
int  doa_legenda_tempos(int fonte, int k, double *t, int max);
/* What the two containers say: another cut (s, the other version longer > 0),
 * frame rates that differ, and a known film/TV speed (ppm, 0 none). 0 = not
 * known yet (nothing opened). */
int  doa_diferenca(int *corte, int *velocidade, int *ppm);

/* The edit map's types (doadorleg_mapa). The app does not use the map: the
 * converter's own follow corrects the sync while the film plays. */
#define DOA_MAPA_MAX 12
typedef struct { double ini, fim, desloc; } DoaTrecho;
typedef struct { int n; double k; DoaTrecho t[DOA_MAPA_MAX]; } DoaMapa;

/* An audio track of the playing file (Scan again's list): its container
 * index `si`, language, title and codec. */
typedef struct { int chave, fonte, ord, si, canais; char lingua[16], nome[96], codec[24], codecFonte[24]; } DoaRendicao;
int  doa_faixas_arquivo(DoaRendicao *out, int max);
#endif
