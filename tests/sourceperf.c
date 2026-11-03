#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static unsigned reads;
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
  assert(sizeof nvSourcePerf < 1024);
  if (argc > 1 && !strcmp(argv[1], "--bench")) {
    realClock = 1; frequency = UINT64_C(1000000000);
    uint64_t begin = counter();
    for (int i = 0; i < 2000000; i++) {
      uint64_t tick = sourceperf_begin(); sourceperf_end(NV_SRC_STATE, tick);
    }
    uint64_t elapsed = counter() - begin;
    assert(nvSourcePerf.stage[NV_SRC_STATE].calls == 2000000 && !used);
    printf("sourceperf host: %.1f ns/stage pair, <1 KiB state, no allocations or output in timed spans\n", (double)elapsed / 2000000.0);
    return 0;
  }
  fresh();
  uint64_t parent = sourceperf_begin(); now += 10000;
  uint64_t child = sourceperf_begin(); now += 5000;
  sourceperf_end(NV_SRC_PREPARE, child); sourceperf_end(NV_SRC_APPEND, parent);
  assert(nvSourcePerf.stage[NV_SRC_APPEND].total == 15000);
  assert(nvSourcePerf.stage[NV_SRC_PREPARE].total == 5000);
  sourceperf_report(15, 7, 1950, 1950, 0, 1);
  assert(occurrences("build=source-ui-v1") == 1 && !occurrences("source-perf-slow"));
  uint64_t filter = sourceperf_begin(); now += 60000;
  sourceperf_end(NV_SRC_FILTER, filter); sourceperf_report(75, 7, 1950, 65, 0, 1);
  assert(occurrences("source-perf-slow") == 1 && strstr(output, "filter_ms=60.00"));
  for (int i = 0; i < 180; i++) { now += 16700; sourceperf_report(100+i*17, 7, 1950, 65, 0, 1); }
  assert(occurrences("frames=") == 1 && strstr(output, "filter=1/60.000/60.000"));
  assert(strstr(output, "append=1/15.000/15.000") && strstr(output, "prepare=1/5.000/5.000"));
  assert(!nvSourcePerf.stage[NV_SRC_APPEND].calls && !nvSourcePerf.pending[NV_SRC_FILTER]);
  // Reopening/replacing sources resets measurements but not the log throttle.
  fresh(); sourceperf_report(0, 1, 100, 100, 0, 0);
  now += 60000; sourceperf_report(60, 1, 100, 100, 0, 0);
  assert(occurrences("source-perf-slow") == 1);
  sourceperf_reset(); assert(!nvSourcePerf.frames && nvSourcePerf.hasSlow);
  uint64_t slow = sourceperf_begin(); now += 60000; sourceperf_end(NV_SRC_LAYOUT, slow);
  sourceperf_report(120, 2, 2000, 65, 1, 8);
  assert(occurrences("source-perf-slow") == 1 && occurrences("build=source-ui-v1") == 1);
  fresh(); sourceperf_report(0, 3, 2000, 2000, 0, 0);
  for (int i = 0; i < 600; i++) { now += 50000; sourceperf_report(i*50, 3, 2000, 2000, 0, 0); }
  assert(occurrences("source-perf-slow") == 30 && occurrences("frames=") == 10);
  // Invalid stages, backward counters and unavailable frequency are harmless.
  unsigned long long calls = nvSourcePerf.stage[NV_SRC_STATE].calls;
  sourceperf_end(-1, now); sourceperf_end(NV_SRC_COUNT, now);
  sourceperf_end(NV_SRC_STATE, now+1);
  assert(nvSourcePerf.stage[NV_SRC_STATE].calls == calls);
  now = 2; sourceperf_report(2, 3, 2000, 2000, 0, 0);
  assert(nvSourcePerf.frames == 1 && nvSourcePerf.start == now);
  fresh(); frequency = 0; sourceperf_report(0, 0, 0, 0, -1, -1); assert(!used);
  puts("PASS: inclusive timing, 3-second summaries, one slow report/second, reset, rollback, zero-frequency and bounded state");
#else
  (void)argc; (void)argv;
  for (int i = 0; i < 100000; i++) {
    uint64_t tick = sourceperf_begin(); assert(tick == 0);
    sourceperf_end(NV_SRC_DRAW, tick); sourceperf_report(i, 1, 2000, 65, 0, 1);
  }
  sourceperf_reset(); assert(!reads);
  puts("PASS: ordinary builds make no source timing clock reads or log writes");
#endif
  return 0;
}
