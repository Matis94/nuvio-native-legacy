/* Real demux/audio setup: DTS Off must not instantiate AAC/AC3 conversion,
 * including provisional DTS discovered before the selected track is mapped. */
#include "../src/dts/dts_engine.c"
#define main baseline_engine_main
#include "dts_engine.c"
#undef main

static void original_audio(DtsEngine *e) {
  assert(e->native_dts && e->passthrough && !e->dec && !e->enc);
  assert(!strcmp(dts_engine_info(e)->audio_codec,"dts"));
}
int main(int argc,char **argv) {
  assert(argc==3);
  for(int fixed=0;fixed<=2;fixed++) {
    DtsEngine *e=dts_engine_create();assert(e);
    dts_engine_dv(e,1);dts_engine_dv_audio(e,fixed);dts_engine_native_dts(e,1);
    check(e,dts_engine_open(e,argv[1],HEADERS,-1,0,0));original_audio(e);
    FILE *out=fixed==0?fopen(argv[2],"wb"):NULL;
    if(!fixed)assert(out);
    DtsFrame f;int result,audios=0,videos=0;
    while((result=dts_engine_next(e,&f))>0) {
      if(f.kind==DTS_AUDIO) {audios++;if(out)assert(fwrite(f.data,1,f.size,out)==f.size);}
      if(f.kind==DTS_VIDEO)videos++;
    }
    check(e,result);assert(audios && videos);if(out)fclose(out);
    check(e,dts_engine_seek(e,0.4));original_audio(e);
    check(e,dts_engine_audio(e,2,0,0));
    assert(!strcmp(e->info.audio_codec,fixed==1?"aac":"ac3"));
    check(e,dts_engine_audio(e,1,0,0));original_audio(e);
    check(e,dts_engine_open(e,argv[1],HEADERS,-1,0,0));original_audio(e);
    dts_engine_native_dts(e,0);
    check(e,dts_engine_open(e,argv[1],HEADERS,-1,0,0));
    assert(e->dec && e->enc && strcmp(e->info.audio_codec,"dts"));
    dts_engine_cancel(e);assert(dts_engine_next(e,&f)<0);dts_engine_destroy(e);
  }
  puts("PASS original DTS policy: exact packets, no app decoder/encoder, fixed-output precedence, seek, audio switch, reopen, reset and cancellation");
  return 0;
}
