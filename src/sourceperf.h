// Render-thread source-sheet timings. Opt-in diagnostic builds only.
// All stages are inclusive; never add a parent to its nested stages.
#ifndef NV_SOURCEPERF_H
#define NV_SOURCEPERF_H
#include <stdint.h>

enum {
  NV_SRC_APPEND, NV_SRC_PREPARE, NV_SRC_MERGE, NV_SRC_BADGES,
  NV_SRC_PROVIDERS, NV_SRC_STATE, NV_SRC_FILTER, NV_SRC_AUTO,
  NV_SRC_LAYOUT, NV_SRC_MEASURE, NV_SRC_UPDATE, NV_SRC_DRAW,
  NV_SRC_SHELL, NV_SRC_BACKGROUND, NV_SRC_ISLAND, NV_SRC_APP_UPDATE,
  NV_SRC_THEME, NV_SRC_CLEAR, NV_SRC_APP_DRAW, NV_SRC_FINISH, NV_SRC_PRESENT,
  NV_SRC_COUNT
};

#ifdef NV_SOURCE_PERF_DIAG
#include <stdio.h>
#include <string.h>
#ifndef NV_SOURCE_PERF_COUNTER
#define NV_SOURCE_PERF_COUNTER() SDL_GetPerformanceCounter()
#define NV_SOURCE_PERF_FREQUENCY() SDL_GetPerformanceFrequency()
#endif
#ifndef NV_SOURCE_PERF_PRINT
#define NV_SOURCE_PERF_PRINT(...) printf(__VA_ARGS__)
#endif
typedef struct { uint64_t total, max, calls; } NvSourcePerfStage;
// Bounded 0.1 ms histogram. The final bucket contains gaps >= 100 ms;
// percentiles in it are reported as 100 ms (a lower bound), never discarded.
#define NV_SOURCE_PACING_BINS 1001
typedef struct {
  NvSourcePerfStage stage[NV_SRC_COUNT];
  uint64_t pending[NV_SRC_COUNT], frequency, start, lastFrame, frameMax;
  uint64_t frames, lastSlow;
  uint32_t pacing[NV_SOURCE_PACING_BINS];
  uint64_t pacingSamples, over20, over25, over33;
  int banner, hasSlow;
} NvSourcePerfState;
// One render-thread state shared by the list and its surrounding scene.
// The list module owns it; host fixtures may define the same owner explicitly.
#ifdef NV_SOURCE_PERF_DEFINE
NvSourcePerfState nvSourcePerf;
#else
extern NvSourcePerfState nvSourcePerf;
#endif
static const char *const nvSourcePerfNames[NV_SRC_COUNT] = {
  "append", "prepare", "merge", "badges", "providers", "state", "filter",
  "auto", "layout", "measure", "update", "draw", "shell", "background",
  "island", "app_update", "theme", "clear", "app_draw", "finish", "present"
};
#define SOURCEPERF_SCENE_BEGIN(stage) \
  uint64_t perfSource##stage = stream_folha_aberta() ? sourceperf_begin() : 0
#define SOURCEPERF_SCENE_END(stage) \
  do { if (perfSource##stage) sourceperf_end(stage, perfSource##stage); } while (0)
static inline uint64_t sourceperf_begin(void) { return NV_SOURCE_PERF_COUNTER(); }
static inline void sourceperf_end(int stage, uint64_t start) {
  uint64_t now = NV_SOURCE_PERF_COUNTER();
  if (stage < 0 || stage >= NV_SRC_COUNT || now < start) return;
  uint64_t delta = now - start;
  NvSourcePerfStage *s = &nvSourcePerf.stage[stage];
  s->total += delta; s->calls++;
  if (delta > s->max) s->max = delta;
  nvSourcePerf.pending[stage] += delta;
}
static inline void sourceperf_reset(void) {
  int banner = nvSourcePerf.banner;
  uint64_t frequency = nvSourcePerf.frequency;
  uint64_t lastSlow = nvSourcePerf.lastSlow;
  int hasSlow = nvSourcePerf.hasSlow;
  memset(&nvSourcePerf, 0, sizeof nvSourcePerf);
  nvSourcePerf.banner = banner; nvSourcePerf.frequency = frequency;
  nvSourcePerf.lastSlow = lastSlow; nvSourcePerf.hasSlow = hasSlow;
}
static inline double sourceperf_ms(uint64_t ticks) {
  return nvSourcePerf.frequency ? (double)ticks * 1000.0 / (double)nvSourcePerf.frequency : 0;
}
static inline void sourceperf_pacing_add(uint64_t gap) {
  double ms = sourceperf_ms(gap);
  unsigned bin = ms >= 100.0 ? NV_SOURCE_PACING_BINS - 1 : (unsigned)(ms * 10.0);
  nvSourcePerf.pacing[bin]++;
  nvSourcePerf.pacingSamples++;
  nvSourcePerf.over20 += ms > 20.0;
  nvSourcePerf.over25 += ms > 25.0;
  nvSourcePerf.over33 += ms > 33.0;
}
static inline double sourceperf_percentile(unsigned percent) {
  uint64_t n = nvSourcePerf.pacingSamples;
  if (!n) return 0;
  uint64_t rank = n / 100 * percent + (n % 100 * percent + 99) / 100;
  uint64_t seen = 0;
  for (unsigned i = 0; i < NV_SOURCE_PACING_BINS; i++) {
    seen += nvSourcePerf.pacing[i];
    if (seen >= rank) return (double)i / 10.0;
  }
  return 100.0;
}
static inline void sourceperf_report(unsigned tick, unsigned generation, int total,
                                      int shown, int tab, int focus) {
  uint64_t now = NV_SOURCE_PERF_COUNTER();
  if (!nvSourcePerf.frequency) nvSourcePerf.frequency = NV_SOURCE_PERF_FREQUENCY();
  if (!nvSourcePerf.frequency) return;
  if (nvSourcePerf.lastFrame && now < nvSourcePerf.lastFrame) {
    sourceperf_reset(); nvSourcePerf.hasSlow = 0;
  }
  if (!nvSourcePerf.banner) {
    NV_SOURCE_PERF_PRINT("[source-perf] build=source-ui-v2 units=calls/avg_ms/max_ms stages=inclusive slow_limit=1/s report=3s pacing=update_interval hist_step_ms=0.1 hist_cap_ms=100 capped_percentiles=lower_bound\n");
    nvSourcePerf.banner = 1;
  }
  if (!nvSourcePerf.lastFrame) nvSourcePerf.start = now;
  uint64_t gap = nvSourcePerf.lastFrame && now >= nvSourcePerf.lastFrame ? now - nvSourcePerf.lastFrame : 0;
  // Exclude the synthetic first zero gap, but include real zero intervals.
  if (nvSourcePerf.frames) sourceperf_pacing_add(gap);
  nvSourcePerf.lastFrame = now;
  if (gap > nvSourcePerf.frameMax) nvSourcePerf.frameMax = gap;
  nvSourcePerf.frames++;
  int slow = sourceperf_ms(gap) >= 50.0;
  for (int i = 0; i < NV_SRC_COUNT; i++)
    if (sourceperf_ms(nvSourcePerf.pending[i]) >= 50.0) slow = 1;
  if (slow && (!nvSourcePerf.hasSlow || now - nvSourcePerf.lastSlow >= nvSourcePerf.frequency)) {
    char line[2048];
    size_t used = (size_t)snprintf(line, sizeof line,
      "[source-perf-slow] t=%u generation=%u total=%d shown=%d tab=%d focus=%d frame_ms=%.2f",
      tick, generation, total, shown, tab, focus, sourceperf_ms(gap));
    for (int i = 0; i < NV_SRC_COUNT && used < sizeof line; i++) {
      int wrote = snprintf(line + used, sizeof line - used, " %s_ms=%.2f", nvSourcePerfNames[i],
                           sourceperf_ms(nvSourcePerf.pending[i]));
      if (wrote < 0 || (size_t)wrote >= sizeof line - used) break;
      used += (size_t)wrote;
    }
    NV_SOURCE_PERF_PRINT("%s\n", line);
    nvSourcePerf.lastSlow = now; nvSourcePerf.hasSlow = 1;
  }
  memset(nvSourcePerf.pending, 0, sizeof nvSourcePerf.pending);
  uint64_t elapsed = now >= nvSourcePerf.start ? now - nvSourcePerf.start : 0;
  if (sourceperf_ms(elapsed) < 3000.0) return;
  char line[2048];
  size_t used = (size_t)snprintf(line, sizeof line,
    "[source-perf] t=%u generation=%u total=%d shown=%d tab=%d focus=%d frames=%llu fps=%.2f frame_max_ms=%.2f",
    tick, generation, total, shown, tab, focus, (unsigned long long)nvSourcePerf.frames,
    elapsed ? (double)(nvSourcePerf.frames - 1) * (double)nvSourcePerf.frequency / (double)elapsed : 0,
    sourceperf_ms(nvSourcePerf.frameMax));
  for (int i = 0; i < NV_SRC_COUNT && used < sizeof line; i++) {
    const NvSourcePerfStage *s = &nvSourcePerf.stage[i];
    int wrote = snprintf(line + used, sizeof line - used, " %s=%llu/%.3f/%.3f", nvSourcePerfNames[i],
      (unsigned long long)s->calls, s->calls ? sourceperf_ms(s->total) / (double)s->calls : 0, sourceperf_ms(s->max));
    if (wrote < 0 || (size_t)wrote >= sizeof line - used) break;
    used += (size_t)wrote;
  }
  NV_SOURCE_PERF_PRINT("%s\n", line);
  // Separate bounded line: no raw frame history, per-frame printing or allocation.
  // These are UI update intervals, not physical TV presentation timestamps.
  NV_SOURCE_PERF_PRINT(
    "[source-pacing] t=%u generation=%u total=%d shown=%d tab=%d focus=%d samples=%llu p50_ms=%.1f p95_ms=%.1f p99_ms=%.1f max_ms=%.2f gt20=%llu gt25=%llu gt33=%llu ge100=%u\n",
    tick, generation, total, shown, tab, focus,
    (unsigned long long)nvSourcePerf.pacingSamples,
    sourceperf_percentile(50), sourceperf_percentile(95), sourceperf_percentile(99),
    sourceperf_ms(nvSourcePerf.frameMax), (unsigned long long)nvSourcePerf.over20,
    (unsigned long long)nvSourcePerf.over25, (unsigned long long)nvSourcePerf.over33,
    nvSourcePerf.pacing[NV_SOURCE_PACING_BINS - 1]);
  memset(nvSourcePerf.pacing, 0, sizeof nvSourcePerf.pacing);
  nvSourcePerf.pacingSamples = nvSourcePerf.over20 = nvSourcePerf.over25 = nvSourcePerf.over33 = 0;
  memset(nvSourcePerf.stage, 0, sizeof nvSourcePerf.stage);
  nvSourcePerf.start = now; nvSourcePerf.frames = 1; nvSourcePerf.frameMax = 0;
}
#else
#define SOURCEPERF_SCENE_BEGIN(stage) do {} while (0)
#define SOURCEPERF_SCENE_END(stage) do {} while (0)
static inline uint64_t sourceperf_begin(void) { return 0; }
static inline void sourceperf_end(int stage, uint64_t start) { (void)stage; (void)start; }
static inline void sourceperf_reset(void) {}
static inline void sourceperf_report(unsigned tick, unsigned generation, int total, int shown, int tab, int focus) {
  (void)tick; (void)generation; (void)total; (void)shown; (void)tab; (void)focus;
}
#endif
#endif

