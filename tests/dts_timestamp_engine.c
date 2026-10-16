/* Real FFmpeg resampler guard with controlled decoded timestamps. */
#define NV_DTS_FFMPEG
#include "../src/dts/dts_engine.c"
#include <assert.h>
static void check(int timestamp_ms,int expected_failure) {
 DtsEngine e={0};AVFrame *f=av_frame_alloc();assert(f);
 e.enc=avcodec_alloc_context3(NULL);e.dec=avcodec_alloc_context3(NULL);assert(e.enc&&e.dec);
 e.enc->time_base=(AVRational){1,48000};e.enc->sample_fmt=AV_SAMPLE_FMT_FLTP;
 av_channel_layout_default(&e.enc->ch_layout,2);e.dec->pkt_timebase=(AVRational){1,1000};
 e.fifo=av_audio_fifo_alloc(AV_SAMPLE_FMT_FLTP,2,4096);assert(e.fifo);e.timeline=1;
 f->sample_rate=48000;f->format=AV_SAMPLE_FMT_FLTP;f->nb_samples=1024;
 av_channel_layout_default(&f->ch_layout,2);f->best_effort_timestamp=timestamp_ms;
 assert(av_frame_get_buffer(f,0)>=0);av_samples_set_silence(f->extended_data,0,f->nb_samples,2,AV_SAMPLE_FMT_FLTP);
 int result=resample(&e,f);
 if(expected_failure) {assert(result<0&&!strcmp(e.error,DTS_ENGINE_TIMESTAMP_DISCONTINUITY));assert(!e.sample_pts&&!av_audio_fifo_size(e.fifo));}
 else assert(result==1024&&av_audio_fifo_size(e.fifo)==1024);
 swr_free(&e.swr);av_audio_fifo_free(e.fifo);av_channel_layout_uninit(&e.input_layout);
 avcodec_free_context(&e.enc);avcodec_free_context(&e.dec);av_frame_free(&f);
}
int main(void) {
 check(0,0);check(100,0);check(-100,0);check(101,1);check(-101,1);check(263800,1);
 puts("PASS: real timestamp guard retains 100 ms boundary and logs forward/backward gaps");
}
