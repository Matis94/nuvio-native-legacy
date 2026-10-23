/* Real HTTP queue ownership: an interior cached seek transfers, never refetches. */
#define NV_DTS_FFMPEG
#define NV_DTS_WEBOS_RECOVERY
#include "../src/dts/dts_engine.c"
#include <assert.h>
#include <unistd.h>
static void check(DtsEngine *e,int64_t at) {
  uint8_t out[4096];assert(seek_range(e,at,SEEK_SET)==at);
  assert(read_range(e,out,sizeof out)==sizeof out);
  for(size_t i=0;i<sizeof out;i++) assert(out[i]==(uint8_t)((at+i)%251));
}
int main(int argc,char **argv) {
  assert(argc==2);
  // Pending interval later completes as a short response, before the seek
  // target. Restart at the exact target instead of copying a negative length.
  {
    DtsEngine *e=dts_engine_create();assert(e);
    char url[1024];const char *path=strrchr(argv[1],'/');assert(path);
    snprintf(url,sizeof url,"%.*s/slow%s",(int)(path-argv[1]),argv[1],path);
    e->url=strdup(url);e->headers=strdup("Authorization: Bearer engine-test\nX-Fixture: yes");
    e->range_bytes=RANGE_BYTES_DV;e->range_slots=2;
    assert(start_ranges(e));
    pthread_mutex_lock(&e->range_lock);queue_ranges(e);pthread_mutex_unlock(&e->range_lock);
    check(e,1024*1024+123);
    dts_engine_destroy(e);
    puts("PASS cached pending seek past a short HTTP prefix");
  }
  for(int mib=64;mib<=256;mib*=2) {
    DtsEngine *e=dts_engine_create();assert(e);
    e->url=strdup(argv[1]);e->headers=strdup("Authorization: Bearer engine-test\nX-Fixture: yes");
    e->range_bytes=RANGE_BYTES_DV;e->range_slots=mib/4;
    check(e,0);
    int full=0;
    for(int n=0;n<2000;n++) {
      pthread_mutex_lock(&e->range_lock);full=1;
      for(int i=0;i<e->range_slots;i++) full&=e->ranges[i].state==3 && e->ranges[i].data!=NULL;
      pthread_mutex_unlock(&e->range_lock);if(full)break;usleep(10000);
    }
    assert(full);
    for(int slot=3;slot<=11;slot+=4) {
      pthread_mutex_lock(&e->range_lock);
      DtsRangeJob *j=&e->ranges[slot];
      int64_t start=j->start,at=start+123;
      char *payload=j->data;long size=j->count;
      pthread_mutex_unlock(&e->range_lock);
      assert(payload && size>8192);
      // FFmpeg can position at an uncached index/EOF and return WITHOUT a read.
      // That probe must not cancel prefetch or free the cached seek target.
      int64_t probe = (int64_t)(e->range_slots + 10) * e->range_bytes;
      assert(seek_range(e,probe,SEEK_SET)==probe);
      pthread_mutex_lock(&e->range_lock);
      assert(j->data==payload && j->state==3);
      pthread_mutex_unlock(&e->range_lock);
      assert(seek_range(e,at,SEEK_SET)==at);
      // Old code cleared this exact payload on an interior seek.
      pthread_mutex_lock(&e->range_lock);assert(j->data==payload && j->state==3);pthread_mutex_unlock(&e->range_lock);
      check(e,at);
      assert(e->cache==payload && e->cache_start==start && e->cache_size==size);
      check(e,at+17); // Small backward/forward reposition inside the active range.
      int64_t bytes=e->cache_size;
      pthread_mutex_lock(&e->range_lock);
      for(int i=0;i<e->range_slots;i++) if(e->ranges[i].data)bytes+=e->ranges[i].count;
      pthread_mutex_unlock(&e->range_lock);
      assert(bytes<=(int64_t)(e->range_slots+1)*e->range_bytes);
    }
    // A real read outside the old window still replaces/refills the bounded
    // queue at the demanded offset; deferring eviction cannot return old bytes.
    check(e,(int64_t)(e->range_slots+10)*e->range_bytes+321);
    assert(e->cache_start==(int64_t)(e->range_slots+10)*e->range_bytes+321);
    dts_engine_destroy(e);
    printf("PASS %d MiB: index positioning retains cached ranges, exact seek bytes, original allocation transferred, unchanged payload cap\n",mib);
  }
}
