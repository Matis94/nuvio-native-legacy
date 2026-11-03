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
  uint32_t gaps[NV_SOURCE_PACING_BINS];
  uint64_t frames, samples, gt20, gt25, gt33, maxGap, callTotal, callMax;
  uint64_t playerFrames, loadingFrames, playerUiFrames, modeChanges;
} NvSourceSwapWindow;
typedef struct {
  uint64_t pairs, moving, holds, fastHolds, reversals, rebases;
  double distance, stepMax, stepChangeMax, speedTotal, speedMax, dtMin, dtMax;
  double first, last;
} NvSourceMotionWindow;
typedef struct {
  double pos, velocity, dt;
  unsigned generation;
  int tab, shown, valid, rebase;
} NvSourceMotionPoint;
typedef struct {
  NvSourcePerfStage stage[NV_SRC_COUNT];
  uint64_t pending[NV_SRC_COUNT], frequency, start, lastFrame, frameMax;
  uint64_t frames, lastSlow;
  uint32_t pacing[NV_SOURCE_PACING_BINS];
  uint64_t pacingSamples, over20, over25, over33;
  NvSourceSwapWindow swap;
  NvSourceMotionWindow motion;
  NvSourceMotionPoint point, previousPoint;
  uint64_t lastSwap;
  double lastStep;
  int lastStepValid, lastPlayer, lastLoading, lastPlayerUi;
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
#define SOURCEPERF_MOTION(open, pos, velocity, dt, generation, tab, shown) \
  sourceperf_motion(open, pos, velocity, dt, generation, tab, shown)
#define SOURCEPERF_SWAP_BEGIN() \
  uint64_t perfSourceSwap = stream_folha_aberta() ? sourceperf_begin() : 0; \
  do { if (!perfSourceSwap) sourceperf_swap_idle(); } while (0)
#define SOURCEPERF_SWAP_END(playerUi, player, loading) \
  do { if (perfSourceSwap) sourceperf_swap_end(perfSourceSwap, playerUi, player, loading); } while (0)
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
static inline double sourceperf_hist_percentile(const uint32_t *hist, uint64_t n, unsigned percent) {
  if (!n) return 0;
  uint64_t rank = n / 100 * percent + (n % 100 * percent + 99) / 100;
  uint64_t seen = 0;
  for (unsigned i = 0; i < NV_SOURCE_PACING_BINS; i++) {
    seen += hist[i];
    if (seen >= rank) return (double)i / 10.0;
  }
  return 100.0;
}
static inline double sourceperf_percentile(unsigned percent) {
  return sourceperf_hist_percentile(nvSourcePerf.pacing, nvSourcePerf.pacingSamples, percent);
}
static inline double sourceperf_abs(double x) { return x < 0 ? -x : x; }
static inline void sourceperf_swap_idle(void) {
  nvSourcePerf.lastSwap = 0;
  nvSourcePerf.point.valid = nvSourcePerf.previousPoint.valid = 0;
  nvSourcePerf.lastStepValid = 0;
}
static inline void sourceperf_motion(int open, double pos, double velocity, double dt,
                                     unsigned generation, int tab, int shown) {
  if (!open) { sourceperf_swap_idle(); return; }
  NvSourceMotionPoint *p = &nvSourcePerf.point;
  p->pos = pos; p->velocity = velocity; p->dt = dt;
  p->generation = generation; p->tab = tab; p->shown = shown; p->valid = 1;
  // Do not mistake filter/layout replacement or focus anchoring for scroll jitter.
  p->rebase = nvSourcePerf.pending[NV_SRC_APPEND] || nvSourcePerf.pending[NV_SRC_FILTER] ||
              nvSourcePerf.pending[NV_SRC_LAYOUT];
}
static inline void sourceperf_swap_end(uint64_t start, int playerUi, int player, int loading) {
  uint64_t now = NV_SOURCE_PERF_COUNTER();
  if (!nvSourcePerf.frequency) nvSourcePerf.frequency = NV_SOURCE_PERF_FREQUENCY();
  if (!nvSourcePerf.frequency || now < start) { sourceperf_swap_idle(); return; }
  NvSourceSwapWindow *s = &nvSourcePerf.swap;
  NvSourceMotionPoint *p = &nvSourcePerf.point, *prev = &nvSourcePerf.previousPoint;
  NvSourceMotionWindow *m = &nvSourcePerf.motion;
  int modeChanged = nvSourcePerf.lastSwap &&
    (player != nvSourcePerf.lastPlayer || loading != nvSourcePerf.lastLoading || playerUi != nvSourcePerf.lastPlayerUi);
  if (modeChanged) { s->modeChanges++; nvSourcePerf.lastSwap = 0; prev->valid = 0; }
  nvSourcePerf.lastPlayer = player; nvSourcePerf.lastLoading = loading; nvSourcePerf.lastPlayerUi = playerUi;
  s->frames++; s->playerFrames += !!player; s->loadingFrames += !!loading; s->playerUiFrames += !!playerUi;
  uint64_t call = now - start;
  s->callTotal += call; if (call > s->callMax) s->callMax = call;
  uint64_t gap = nvSourcePerf.lastSwap && now >= nvSourcePerf.lastSwap ? now - nvSourcePerf.lastSwap : 0;
  int pair = nvSourcePerf.lastSwap && now > nvSourcePerf.lastSwap;
  if (nvSourcePerf.lastSwap && now >= nvSourcePerf.lastSwap) {
    double ms = sourceperf_ms(gap);
    unsigned bin = ms >= 100 ? NV_SOURCE_PACING_BINS - 1 : (unsigned)(ms * 10);
    s->gaps[bin]++; s->samples++;
    s->gt20 += ms > 20; s->gt25 += ms > 25; s->gt33 += ms > 33;
    if (gap > s->maxGap) s->maxGap = gap;
  }
  if (p->valid) {
    if (pair && prev->valid && !p->rebase && p->generation == prev->generation &&
        p->tab == prev->tab && p->shown == prev->shown) {
      double step = p->pos - prev->pos, amount = sourceperf_abs(step);
      double speed = amount * (double)nvSourcePerf.frequency / (double)gap;
      if (!m->pairs) { m->first = prev->pos; m->dtMin = p->dt; m->dtMax = p->dt; }
      m->pairs++; m->last = p->pos;
      if (p->dt < m->dtMin) m->dtMin = p->dt;
      if (p->dt > m->dtMax) m->dtMax = p->dt;
      if (amount > .02) {
        m->moving++; m->distance += amount; m->speedTotal += speed;
        if (amount > m->stepMax) m->stepMax = amount;
        if (speed > m->speedMax) m->speedMax = speed;
      } else if (sourceperf_abs(p->velocity) > 5) {
        m->holds++; m->fastHolds += sourceperf_abs(p->velocity) >= 60;
      }
      if (nvSourcePerf.lastStepValid) {
        double change = sourceperf_abs(step - nvSourcePerf.lastStep);
        if (change > m->stepChangeMax) m->stepChangeMax = change;
        if (amount > .02 && sourceperf_abs(nvSourcePerf.lastStep) > .02 && step * nvSourcePerf.lastStep < 0) m->reversals++;
      }
      nvSourcePerf.lastStep = step; nvSourcePerf.lastStepValid = 1;
    } else { m->rebases++; nvSourcePerf.lastStepValid = 0; }
    *prev = *p; p->valid = 0;
  } else { prev->valid = 0; nvSourcePerf.lastStepValid = 0; }
  nvSourcePerf.lastSwap = now;
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
  // Swap return intervals are submission timing, not physical panel timestamps.
  const NvSourceSwapWindow *s = &nvSourcePerf.swap;
  const NvSourceMotionWindow *m = &nvSourcePerf.motion;
  NV_SOURCE_PERF_PRINT(
    "[source-swap] t=%u generation=%u frames=%llu player=%llu loading=%llu player_ui=%llu mode_changes=%llu samples=%llu p50_ms=%.1f p95_ms=%.1f p99_ms=%.1f max_ms=%.2f gt20=%llu gt25=%llu gt33=%llu ge100=%u call_avg_ms=%.3f call_max_ms=%.3f\n",
    tick, generation, (unsigned long long)s->frames, (unsigned long long)s->playerFrames,
    (unsigned long long)s->loadingFrames, (unsigned long long)s->playerUiFrames, (unsigned long long)s->modeChanges,
    (unsigned long long)s->samples, sourceperf_hist_percentile(s->gaps, s->samples, 50),
    sourceperf_hist_percentile(s->gaps, s->samples, 95), sourceperf_hist_percentile(s->gaps, s->samples, 99),
    sourceperf_ms(s->maxGap), (unsigned long long)s->gt20, (unsigned long long)s->gt25,
    (unsigned long long)s->gt33, s->gaps[NV_SOURCE_PACING_BINS - 1],
    s->frames ? sourceperf_ms(s->callTotal) / (double)s->frames : 0, sourceperf_ms(s->callMax));
  NV_SOURCE_PERF_PRINT(
    "[source-motion] t=%u generation=%u tab=%d shown=%d pairs=%llu moving=%llu holds=%llu fast_holds=%llu reversals=%llu rebases=%llu from_px=%.2f to_px=%.2f step_avg_px=%.3f step_max_px=%.3f step_change_max_px=%.3f speed_avg_px_s=%.2f speed_max_px_s=%.2f anim_dt_min_ms=%.3f anim_dt_max_ms=%.3f\n",
    tick, generation, tab, shown, (unsigned long long)m->pairs, (unsigned long long)m->moving,
    (unsigned long long)m->holds, (unsigned long long)m->fastHolds, (unsigned long long)m->reversals,
    (unsigned long long)m->rebases, m->first, m->last, m->moving ? m->distance / (double)m->moving : 0,
    m->stepMax, m->stepChangeMax, m->moving ? m->speedTotal / (double)m->moving : 0,
    m->speedMax, m->dtMin * 1000, m->dtMax * 1000);
  memset(&nvSourcePerf.swap, 0, sizeof nvSourcePerf.swap);
  memset(&nvSourcePerf.motion, 0, sizeof nvSourcePerf.motion);
  memset(nvSourcePerf.pacing, 0, sizeof nvSourcePerf.pacing);
  nvSourcePerf.pacingSamples = nvSourcePerf.over20 = nvSourcePerf.over25 = nvSourcePerf.over33 = 0;
  memset(nvSourcePerf.stage, 0, sizeof nvSourcePerf.stage);
  nvSourcePerf.start = now; nvSourcePerf.frames = 1; nvSourcePerf.frameMax = 0;
}
#else
#define SOURCEPERF_SCENE_BEGIN(stage) do {} while (0)
#define SOURCEPERF_SCENE_END(stage) do {} while (0)
#define SOURCEPERF_MOTION(open, pos, velocity, dt, generation, tab, shown) do {} while (0)
#define SOURCEPERF_SWAP_BEGIN() do {} while (0)
#define SOURCEPERF_SWAP_END(playerUi, player, loading) do {} while (0)
static inline uint64_t sourceperf_begin(void) { return 0; }
static inline void sourceperf_end(int stage, uint64_t start) { (void)stage; (void)start; }
static inline void sourceperf_reset(void) {}
static inline void sourceperf_report(unsigned tick, unsigned generation, int total, int shown, int tab, int focus) {
  (void)tick; (void)generation; (void)total; (void)shown; (void)tab; (void)focus;
}
#endif
#endif


