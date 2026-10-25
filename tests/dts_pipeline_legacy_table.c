/* Simulate an installed ABI-2 adapter with no optional audio tail. Keep the
 * real adapter underneath, so the ordinary lifecycle/ABI tests still apply. */
#include "dts/adapter/adapter.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
static int forbidden_audio(void *p,const DtsMediaInfo *m) {
  (void)p;(void)m;assert(!"Caller read beyond legacy table size");return 0;
}
const DtsAdapter *nuvio_dts_adapter_v2(void) {
  static DtsAdapter legacy;
  static void *library;
  if(!library) {
    const char *dir=getenv("DTS_TEST_REAL_ADAPTER_DIR");assert(dir);
    char path[4096];int n=snprintf(path,sizeof path,"%s/dts-starfish-webos%d.so",dir,DTS_TEST_GENERATION);
    assert(n>0 && n<(int)sizeof path);
    library=dlopen(path,RTLD_NOW|RTLD_LOCAL);assert(library);
    const DtsAdapter *(*entry)(void);
    *(void **)(&entry)=dlsym(library,"nuvio_dts_adapter_v2");assert(entry);
    const DtsAdapter *real=entry();assert(real && real->size==sizeof *real);
    legacy=*real;legacy.size=offsetof(DtsAdapter,audio_config);
    legacy.audio_config=forbidden_audio;
  }
  return &legacy;
}
