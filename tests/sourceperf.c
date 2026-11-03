#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static unsigned reads;
static int visible, visibilityReads;
int stream_folha_aberta(void) { visibilityReads++; return visible; }
int sourceperf_peer(void);
#ifdef NV_SOURCE_PERF_DIAG
static uint64_t now = 1, frequency = 1000000;
static int realClock;
static char output[65536];
static size_t used;
static uint64_t counter(void) {
  reads++;
  if (!realClock) return now;
  struct timespec t; assert(!clock_gettime(CLOCK_MONOTONIC, &t));
  return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}
static int capture(const char *format, ...) {
  va_list args; va_start(args, format);
  int n = vsnprintf(output + used, sizeof output - used, format, args);
  va_end(args); assert(n >= 0 && (size_t)n < sizeof output - used);
  used += (size_t)n; return n;
}
#define NV_SOURCE_PERF_COUNTER() counter()
#define NV_SOURCE_PERF_FREQUENCY() frequency
#define NV_SOURCE_PERF_PRINT(...) capture(__VA_ARGS__)
#endif
#define NV_SOURCE_PERF_DEFINE
#include "sourceperf.h"
#ifdef NV_SOURCE_PERF_DIAG
static int occurrences(const char *text) {
  int n = 0;
  for (const char *p = output; (p = strstr(p, text)); p += strlen(text)) n++;
  return n;
}
static void fresh(void) {
  memset(&nvSourcePerf, 0, sizeof nvSourcePerf);
  memset(output, 0, sizeof output); used = 0; now = 1;
}
#endif
int main(int argc, char **argv) {
#ifdef NV_SOURCE_PERF_DIAG
  assert(sizeof nvSourcePerf < 6144);
  if (argc > 1 && !strcmp(argv[1], "--bench")) {
    realClock = 1; frequency = UINT64_C(1000000000);
    uint64_t begin = counter();
    for (int i = 0; i < 2000000; i++) {
      uint64_t tick = sourceperf_begin(); sourceperf_end(NV_SRC_STATE, tick);
    }
    uint64_t elapsed = counter() - begin;
    assert(nvSourcePerf.stage[NV_SRC_STATE].calls == 2000000 && !used);
    printf("sourceperf host: %.1f ns/stage pair, <6 KiB state, no allocations or output in timed spans\n", (double)elapsed / 2000000.0);
    begin = counter();
    for (int i = 0; i < 2000000; i++) sourceperf_pacing_add(16670000);
    elapsed = counter() - begin;
    assert(nvSourcePerf.pacingSamples == 2000000 && !used);
    printf("source pacing host: %.1f ns/sample, fixed histogram, no per-frame output\n", (double)elapsed / 2000000.0);
    return 0;
  }
  fresh();
  unsigned before = reads;
  { SOURCEPERF_SCENE_BEGIN(NV_SRC_BACKGROUND); now += 500;
    SOURCEPERF_SCENE_END(NV_SRC_BACKGROUND); }
  assert(reads == before && !nvSourcePerf.stage[NV_SRC_BACKGROUND].calls);
  visible = 1;
  { SOURCEPERF_SCENE_BEGIN(NV_SRC_BACKGROUND); now += 700;
    SOURCEPERF_SCENE_END(NV_SRC_BACKGROUND); }
  assert(nvSourcePerf.stage[NV_SRC_BACKGROUND].total == 700);
  assert(sourceperf_peer() == 2 && nvSourcePerf.stage[NV_SRC_ISLAND].total == 13);
  fresh();
  uint64_t parent = sourceperf_begin(); now += 10000;
  uint64_t child = sourceperf_begin(); now += 5000;
  sourceperf_end(NV_SRC_PREPARE, child); sourceperf_end(NV_SRC_APPEND, parent);
  assert(nvSourcePerf.stage[NV_SRC_APPEND].total == 15000);
  assert(nvSourcePerf.stage[NV_SRC_PREPARE].total == 5000);
  sourceperf_report(15, 7, 1950, 1950, 0, 1);
  assert(occurrences("build=source-ui-v2") == 1 && !occurrences("source-perf-slow"));
  uint64_t filter = sourceperf_begin(); now += 60000;
  sourceperf_end(NV_SRC_FILTER, filter); sourceperf_report(75, 7, 1950, 65, 0, 1);
  assert(occurrences("source-perf-slow") == 1 && strstr(output, "filter_ms=60.00"));
  for (int i = 0; i < 180; i++) { now += 16700; sourceperf_report(100+i*17, 7, 1950, 65, 0, 1); }
  assert(occurrences("frames=") == 1 && occurrences("[source-pacing]") == 1 && strstr(output, "filter=1/60.000/60.000"));
  assert(strstr(output, "append=1/15.000/15.000") && strstr(output, "prepare=1/5.000/5.000"));
  assert(strstr(output, "background=0/0.000/0.000") && strstr(output, "present=0/0.000/0.000"));
  assert(!nvSourcePerf.stage[NV_SRC_APPEND].calls && !nvSourcePerf.pending[NV_SRC_FILTER]);
  // Reopening/replacing sources resets measurements but not the log throttle.
  fresh(); sourceperf_report(0, 1, 100, 100, 0, 0);
  now += 60000; sourceperf_report(60, 1, 100, 100, 0, 0);
  assert(occurrences("source-perf-slow") == 1);
  sourceperf_reset(); assert(!nvSourcePerf.frames && nvSourcePerf.hasSlow);
  uint64_t slow = sourceperf_begin(); now += 60000; sourceperf_end(NV_SRC_LAYOUT, slow);
  sourceperf_report(120, 2, 2000, 65, 1, 8);
  assert(occurrences("source-perf-slow") == 1 && occurrences("build=source-ui-v2") == 1);
  fresh(); sourceperf_report(0, 3, 2000, 2000, 0, 0);
  for (int i = 0; i < 600; i++) { now += 50000; sourceperf_report(i*50, 3, 2000, 2000, 0, 0); }
  assert(occurrences("source-perf-slow") == 30 && occurrences("frames=") == 10 && occurrences("[source-pacing]") == 10);
  // Invalid stages, backward counters and unavailable frequency are harmless.
  unsigned long long calls = nvSourcePerf.stage[NV_SRC_STATE].calls;
  sourceperf_end(-1, now); sourceperf_end(NV_SRC_COUNT, now);
  sourceperf_end(NV_SRC_STATE, now+1);
  assert(nvSourcePerf.stage[NV_SRC_STATE].calls == calls);
  now = 2; sourceperf_report(2, 3, 2000, 2000, 0, 0);
  assert(nvSourcePerf.frames == 1 && nvSourcePerf.start == now);
  assert(!nvSourcePerf.pacingSamples);
  // Known distribution: nearest-rank p50/p95/p99, exact strict thresholds.
  fresh(); frequency = 1000000; nvSourcePerf.frequency = frequency;
  assert(sourceperf_percentile(50) == 0);
  for (int i = 0; i < 90; i++) sourceperf_pacing_add(16000);
  for (int i = 0; i < 5; i++) sourceperf_pacing_add(22000);
  for (int i = 0; i < 4; i++) sourceperf_pacing_add(28000);
  sourceperf_pacing_add(40000);
  assert(sourceperf_percentile(50) == 16 && sourceperf_percentile(95) == 22 && sourceperf_percentile(99) == 28);
  assert(nvSourcePerf.pacingSamples == 100 && nvSourcePerf.over20 == 10 && nvSourcePerf.over25 == 5 && nvSourcePerf.over33 == 1);
  sourceperf_reset(); assert(!nvSourcePerf.pacingSamples);
  sourceperf_pacing_add(20000); sourceperf_pacing_add(20001);
  sourceperf_pacing_add(25000); sourceperf_pacing_add(25001);
  sourceperf_pacing_add(33000); sourceperf_pacing_add(33001);
  assert(nvSourcePerf.over20 == 5 && nvSourcePerf.over25 == 3 && nvSourcePerf.over33 == 1);
  // Non-MHz counters use the same units, retaining zero and overflow gaps.
  sourceperf_reset(); nvSourcePerf.frequency = 1000000000;
  sourceperf_pacing_add(16670000); assert(sourceperf_percentile(50) >= 16.6 && sourceperf_percentile(50) <= 16.7);
  sourceperf_reset(); sourceperf_pacing_add(0); sourceperf_pacing_add(3000000000);
  assert(nvSourcePerf.pacingSamples == 2 && nvSourcePerf.pacing[0] == 1 && nvSourcePerf.pacing[1000] == 1);
  assert(sourceperf_percentile(99) == 100 && nvSourcePerf.over33 == 1);
  // Each boundary interval belongs to exactly one summary; no synthetic zero.
  fresh(); sourceperf_report(0, 8, 1953, 1953, 0, 0);
  for (int i = 0; i < 100; i++) { now += 30000; sourceperf_report((i+1)*30, 8, 1953, 1953, 0, i); }
  assert(occurrences("[source-pacing]") == 1);
  assert(strstr(output, "samples=100 p50_ms=30.0 p95_ms=30.0 p99_ms=30.0 max_ms=30.00 gt20=100 gt25=100 gt33=0 ge100=0"));
  assert(!nvSourcePerf.pacingSamples && !nvSourcePerf.over20 && !nvSourcePerf.pacing[300]);
  now += 40000; sourceperf_report(3040, 8, 1953, 1953, 0, 100);
  assert(nvSourcePerf.pacingSamples == 1 && nvSourcePerf.over33 == 1);
  for (int i = 0; i < 99; i++) { now += 30000; sourceperf_report(3070+i*30, 8, 1953, 1953, 0, 101+i); }
  assert(occurrences("[source-pacing]") == 2);
  assert(strstr(output, "samples=100 p50_ms=30.0 p95_ms=30.0 p99_ms=30.0 max_ms=40.00 gt20=100 gt25=100 gt33=1 ge100=0"));
  fresh(); sourceperf_report(0, 9, 1953, 1953, 0, 0);
  now += 3000000; sourceperf_report(3000, 9, 1953, 1953, 0, 1);
  assert(strstr(output, "samples=1 p50_ms=100.0 p95_ms=100.0 p99_ms=100.0 max_ms=3000.00 gt20=1 gt25=1 gt33=1 ge100=1"));
  fresh(); frequency = 0; sourceperf_report(0, 0, 0, 0, -1, -1); assert(!used);
  puts("PASS: inclusive timing, 3-second summaries, one slow report/second, reset, rollback, zero-frequency and bounded state");
  puts("PASS: pacing percentiles, strict thresholds, counter units, overflow, resets and consecutive report boundaries");
#else
  (void)argc; (void)argv;
  for (int i = 0; i < 100000; i++) {
    uint64_t tick = sourceperf_begin(); assert(tick == 0);
    sourceperf_end(NV_SRC_DRAW, tick); sourceperf_report(i, 1, 2000, 65, 0, 1);
  }
  sourceperf_reset(); assert(!reads);
  { SOURCEPERF_SCENE_BEGIN(NV_SRC_BACKGROUND); SOURCEPERF_SCENE_END(NV_SRC_BACKGROUND); }
  assert(!reads && !visibilityReads);
  assert(sourceperf_peer() == 0 && !visibilityReads);
  puts("PASS: ordinary builds make no source timing clock reads or log writes");
#endif
  return 0;
}

