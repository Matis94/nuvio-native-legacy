/* Automatic sync of another version's audio (src/dts/dts_match.c): a shared
 * "music and effects" bed with different speech on top is found at the right
 * offset; unrelated audio is not trusted. */
#include "../src/dts/dts_match.inc"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned seed = 12345;
static double rnd(void) { seed = seed * 1103515245u + 12345u; return ((seed >> 8) & 0xFFFF) / 65536.0; }

/* Bursts that decay (hits, cuts, music notes) at random places. */
static void bursts(float *e, int n, int count, double amp, unsigned s) {
  seed = s;
  for (int k = 0; k < count; k++) {
    int at = (int)(rnd() * n);
    double a = amp * (0.3 + rnd());
    for (int i = at; i < n && i < at + 60; i++) e[i] += (float)(a * exp(-(i - at) / 12.0));
  }
}

int main(void) {
  enum { N = DTS_MATCH_MAX, SHIFT = 70 };   /* 0.70 s */
  static float bed[N + 400], m[N], d[N], other[N];
  DtsMatchResult r;
  bursts(bed, N + 400, 900, 1.0, 1);
  for (int i = 0; i < N; i++) { m[i] = 1e-4f + bed[i + 200]; d[i] = 1e-4f + bed[i + 200 - SHIFT]; }
  bursts(m, N, 500, 0.6, 2);    /* this file's speech */
  bursts(d, N, 500, 0.6, 3);    /* the dub's speech: elsewhere */
  /* The other version is 0.7 s later: its timestamps need -0.7 s. Same window start. */
  assert(dts_match_offset(m, N, 100.0, d, N, 100.0, 0.0, 10.0, &r) == 1);
  printf("dub 0.70 s late: offset %.3f s score %.2f margin %.2f confident %d\n", r.offset_s, r.score, r.margin, r.confident);
  assert(r.confident && fabs(r.offset_s + 0.70) < 0.02);
  /* Searched around the current offset: the same audio with its timestamps
   * 2 s earlier (the other version's clock) needs +2 s more: +1.30 s. */
  assert(dts_match_offset(m, N, 100.0, d, N, 98.0, 2.0, 10.0, &r) == 1);
  assert(r.confident && fabs(r.offset_s - 1.30) < 0.02);
  /* Unrelated audio: no trusted result. */
  bursts(other, N, 1400, 1.0, 99);
  for (int i = 0; i < N; i++) other[i] += 1e-4f;
  assert(dts_match_offset(m, N, 100.0, other, N, 100.0, 0.0, 10.0, &r) == 1);
  printf("unrelated: score %.2f margin %.2f confident %d\n", r.score, r.margin, r.confident);
  assert(!r.confident);
  /* Too little audio: no comparison at all. */
  assert(dts_match_offset(m, 500, 0, d, 500, 0, 0, 10, &r) == 0);
  puts("dts_match: shared bed found at the right offset, unrelated audio refused");
  return 0;
}
