#define MP4_BD_MAIN split_baseline_main
#include "mp4_bd_engine.c"
#undef MP4_BD_MAIN
/* A P8 recovery must not invoke P7 conversion even when built with libdovi. */
ptrdiff_t __wrap_nv_dv7_rpu(const uint8_t *p,size_t n,uint8_t *out,size_t cap) {
  (void)p;(void)n;(void)out;(void)cap;assert(!"P8 must retain its original metadata");return -1;
}
int main(int argc,char **argv) {
  assert(argc==4);char path[4096],rpu[4096];
  snprintf(rpu,sizeof rpu,"%s/%s-output.nal",argv[1],argv[2]);
  const char *good[]={"p81","p81-inband-nocolr"};
  const char *bad[]={"p84","p8-unknown","p81-extra-video","p81-with-el","p81-no-bl",
    "p81-bad-rpu","p81-p7-rpu","p81-sdr","p5","dual"};
  for(int mode=0;mode<=2;mode++)for(int mib=64;mib<=256;mib*=2) {
    DtsEngine *e=dts_engine_create();assert(e);
    dts_engine_dv(e,1);dts_engine_allow_p7(e,0);dts_engine_mp4_test(e,1);
    dts_engine_dv_audio(e,mode);dts_engine_set_ahead_mib(mib);
    for(size_t i=0;i<sizeof good/sizeof good[0];i++) {
      snprintf(path,sizeof path,"%s/%s-%s.mp4",argv[1],argv[2],good[i]);
      check(e,dts_engine_open(e,path,"",-1,0,0));check_dv(e);
      DtsBufferInfo b;dts_engine_buffer_info(e,&b);
      assert(b.capacity_bytes==(mib/4-4+1)*(4*1024*1024LL-1));
      FILE *out=mode==0 && mib==64 && i==0?fopen(argv[3],"wb"):NULL;
      if(mode==0 && mib==64 && i==0)assert(out);
      assert(drain_split(e,rpu,out)==20);if(out)fclose(out);
      check(e,dts_engine_seek(e,.4));assert(drain_split(e,rpu,NULL)>0);
      check(e,dts_engine_seek(e,0));assert(drain_split(e,rpu,NULL)==20);
      check(e,dts_engine_audio(e,2,0,.4));assert(drain_split(e,rpu,NULL)>0);
      check(e,dts_engine_audio(e,1,0,0));assert(drain_split(e,rpu,NULL)==20);
      check(e,dts_engine_open(e,path,"",-1,0,.4));assert(drain_split(e,rpu,NULL)>0);
    }
    for(size_t i=0;i<sizeof bad/sizeof bad[0];i++) {
      snprintf(path,sizeof path,"%s/%s-%s.mp4",argv[1],argv[2],bad[i]);
      assert(dts_engine_open(e,path,"",-1,0,0)<0);
    }
    snprintf(path,sizeof path,"%s/%s-p81.mp4",argv[1],argv[2]);
    dts_engine_mp4_test(e,0);check(e,dts_engine_open(e,path,"",-1,0,0));
    assert(drain_split(e,rpu,NULL)==20);
    dts_engine_cancel(e);DtsFrame f;assert(dts_engine_next(e,&f)<0);dts_engine_destroy(e);
  }
  dts_engine_set_ahead_mib(0);
  puts("PASS verified P8.1: unchanged RPU, no P7 converter, presets64/128/256, seeks, resume, audio, OFF baseline, invalid/unsupported profile refusal");
  return 0;
}
