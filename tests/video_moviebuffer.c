/* Real native buffer reporting and shutdown, using the existing bus fixture. */
#define main video_acb_fixture_main
#include "video_acb_lg.c"
#undef main
int main(void) {
  dtsSessao=NULL;posSeg=10;durSeg=100;bufferSeg=20;
  movie_snapshot=(DtsBufferInfo){.ready_bytes=200,.capacity_bytes=256,.total=1000};
  movie_active=1;
  assert(fabs(video_buffer_fim()-30)<.001);
  movie_snapshot.ready_bytes=950;
  assert(fabs(video_buffer_fim()-100)<.001);
  movie_snapshot.ready_bytes=0;
  assert(video_buffer_fim()==20);
  movie_snapshot.ready_bytes=200;movie_active=-1;
  assert(video_buffer_fim()==20);
  movie_active=1;dtsSessao=(DtsPlayback *)(uintptr_t)1;
  assert(video_buffer_fim()==20);dtsSessao=NULL;
  movie_active=1;sessao=41;ligado=0;midia[0]=0;acb=0;
  int stops=movie_stops;
  pararSessao();
  assert(movie_stops==stops+1 && !movie_active && movie_stop_session==42);
  puts("video_moviebuffer: native rail estimate, fallback, converter isolation and shutdown passed");
}
