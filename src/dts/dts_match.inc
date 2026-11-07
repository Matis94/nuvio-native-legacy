/* Automatic sync of another version's audio: the pure part (dts_match.h).
 * Included by dts_engine.c (and by its unit test): static, no build list to
 * update. */
#include "dts_match.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Onsets: the rise of the log energy from one block to the next, so loudness
 * differences between the two mixes do not matter. */
static void onsets(const float *e, int n, float *y) {
  double prev = log10((double)e[0] + 1e-10);
  y[0] = 0.0f;
  for (int i = 1; i < n; i++) {
    double v = log10((double)e[i] + 1e-10), d = v - prev;
    y[i] = d > 0.0 ? (float)d : 0.0f;
    prev = v;
  }
}
/* Pearson correlation of a[0..w) and b[0..w). */
static double corr(const float *a, const float *b, int w) {
  double sa = 0, sb = 0, saa = 0, sbb = 0, sab = 0;
  for (int i = 0; i < w; i++) { sa += a[i]; sb += b[i]; }
  double ma = sa / w, mb = sb / w;
  for (int i = 0; i < w; i++) {
    double x = a[i] - ma, z = b[i] - mb;
    saa += x * x; sbb += z * z; sab += x * z;
  }
  if (saa <= 1e-12 || sbb <= 1e-12) return -2.0;
  return sab / sqrt(saa * sbb);
}

static int dts_match_offset(const float *main_env, int nm, double t0m,
                     const float *ext_env, int nd, double t0d,
                     double center_s, double radius_s, DtsMatchResult *out) {
  const double step = 1.0 / DTS_MATCH_HZ;
  const int w = 10 * DTS_MATCH_HZ;          /* 10 s of this file, in the middle of what was captured */
  int lags = (int)(radius_s * DTS_MATCH_HZ + 0.5), best = -1, ok = 0;
  float *ym, *yd;
  double *c;
  memset(out, 0, sizeof *out);
  if (nm < w + 2 || nd < w + 2 || lags < 1) return 0;
  ym = malloc(sizeof *ym * (size_t)nm); yd = malloc(sizeof *yd * (size_t)nd);
  c = malloc(sizeof *c * (size_t)(2 * lags + 1));
  if (!ym || !yd || !c) { free(ym); free(yd); free(c); return 0; }
  onsets(main_env, nm, ym); onsets(ext_env, nd, yd);
  int m0 = (nm - w) / 2;
  for (int k = -lags; k <= lags; k++) {
    double o = center_s + k * step;
    /* this file at t pairs with the other at t - offset */
    long js = lround((t0m + m0 * step - o - t0d) / step);
    double v = -2.0;
    if (js >= 0 && js + w <= nd) v = corr(ym + m0, yd + js, w);
    c[k + lags] = v;
    if (v > -2.0) { ok = 1; if (best < 0 || v > c[best]) best = k + lags; }
  }
  if (!ok) { free(ym); free(yd); free(c); return 0; }
  double p = c[best], q = -2.0;
  for (int i = 0; i < 2 * lags + 1; i++)
    if (abs(i - best) > DTS_MATCH_HZ / 4 && c[i] > q) q = c[i];
  /* Sub-block peak from its neighbours (parabola). */
  double frac = 0.0;
  if (best > 0 && best < 2 * lags && c[best - 1] > -2.0 && c[best + 1] > -2.0) {
    double den = c[best - 1] - 2.0 * p + c[best + 1];
    if (fabs(den) > 1e-12) frac = 0.5 * (c[best - 1] - c[best + 1]) / den;
    if (frac > 0.5) frac = 0.5;
    if (frac < -0.5) frac = -0.5;
  }
  out->offset_s = center_s + (best - lags + frac) * step;
  out->score = p;
  out->margin = q > -2.0 ? p - q : p;
  out->confident = p >= 0.25 && out->margin >= 0.06;
  free(ym); free(yd); free(c);
  return 1;
}
