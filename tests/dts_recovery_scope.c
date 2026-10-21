/* Preserve non-webOS request and timestamp behavior when the opt-in is absent. */
#define NV_TEST_LEGACY_RECOVERY
#define main timestamp_fixture_main
#include "dts_timestamp_engine.c"
#undef main
int main(void) {
  av_log_set_level(AV_LOG_ERROR);
  boundary(0,0); boundary(4800,0); boundary(-4800,0);
  boundary(4801,1); boundary(24000,1); boundary(-4801,1);
  (void)runtime_policy;
  DtsEngine e; init(&e,2,0);dts_engine_recovery(&e,1);assert(!e.recovery);cleanup(&e);
  puts("PASS: without webOS recovery, timestamp gaps retain the original rejection and cannot be enabled at runtime");
  return 0;
}
