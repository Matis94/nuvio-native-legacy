// A second translation unit must record into the render thread's one state.
#include <stdint.h>
static unsigned peerReads;
#ifdef NV_SOURCE_PERF_DIAG
static uint64_t peerCounter(void) { return (uint64_t)++peerReads * 13; }
#define NV_SOURCE_PERF_COUNTER() peerCounter()
#define NV_SOURCE_PERF_FREQUENCY() 1000000
#endif
#include "sourceperf.h"
int stream_folha_aberta(void);
int sourceperf_peer(void) {
  unsigned before = peerReads;
  SOURCEPERF_SCENE_BEGIN(NV_SRC_ISLAND);
  SOURCEPERF_SCENE_END(NV_SRC_ISLAND);
  return (int)(peerReads - before);
}
