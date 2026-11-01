/* Deterministic queue-order fixture; production still uses its four lanes. */
#define NV_DTS_FFMPEG
#define NV_DTS_WEBOS_RECOVERY
#define RANGE_WORKERS 1
#include "../src/dts/dts_engine.c"
#include <assert.h>
int main(int argc,char **argv) {
  assert(argc==2);DtsEngine *e=dts_engine_create();assert(e);
  e->url=strdup(argv[1]);e->headers=strdup("Authorization: Bearer engine-test\nX-Fixture: yes");
  e->range_slots=8;
  pthread_mutex_lock(&e->range_lock);queue_ranges(e);
  __atomic_store_n(&e->ranges[4].urgent,2,__ATOMIC_RELAXED);
  pthread_mutex_unlock(&e->range_lock);assert(start_ranges(e));
  pthread_mutex_lock(&e->range_lock);
  e->range_next=(int64_t)e->range_slots*e->range_bytes;
  pthread_mutex_unlock(&e->range_lock);
  unsigned char bytes[512];assert(read_range(e,bytes,(int)sizeof bytes)==(int)sizeof bytes);
  for(int i=0;i<(int)sizeof bytes;i++)assert(bytes[i]==(unsigned char)(i%251));
  pthread_mutex_lock(&e->range_lock);
  DtsRangeJob *gap=&e->ranges[4];
  assert(gap->state==3 && gap->data && gap->count==RANGE_BYTES);
  assert((unsigned char)gap->data[0]==(unsigned char)((4*RANGE_BYTES)%251));
  pthread_mutex_unlock(&e->range_lock);
  uint64_t began=clock_ns(CLOCK_MONOTONIC);
  dts_engine_cancel(e);dts_engine_destroy(e);
  assert(clock_ns(CLOCK_MONOTONIC)-began<UINT64_C(3000000000));
  puts("PASS: awaited gap precedes older speculation; exact bytes and bounded teardown");
}
