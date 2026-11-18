/* Real native buffer reporting and shutdown, using the existing bus fixture. */
#define main video_acb_fixture_main
#include "video_acb_lg.c"
#undef main

static int native_opacity, opacity_calls;
static int fake_subtitle_style(LSHandle *h, const char *uri, const char *payload,
                               Filtro cb, void *ctx, unsigned long *token, void *error) {
  (void)h; (void)cb; (void)ctx; (void)token; (void)error;
  if (strstr(uri, "/setSubtitleCharacterOpacity")) {
    const char *value = strstr(payload, "\"charOpacity\":");
    assert(value && sscanf(value, "\"charOpacity\":%d", &native_opacity) == 1);
    opacity_calls++;
  }
  return 1;
}

static void test_native_subtitle_opacity(void) {
  bus = (LSHandle *)(uintptr_t)1;
  lsCall = fake_subtitle_style;
  ligado = 1;
  snprintf(midia, sizeof midia, "subtitle-fixture");
  VideoLegendaEstilo e = {.tamanho = 120, .corBorda = -1};
  for (int pct = 5; pct <= 100; pct += 5) {
    e.opacidade = leg_opacidade_modo(pct);
    video_legenda_estilo(&e);
    assert(native_opacity == (pct * 255 + 50) / 100 && opacity_calls == pct / 5);
  }
  dtsSessao = (DtsPlayback *)(uintptr_t)1;
  video_legenda_estilo(&e);
  assert(opacity_calls == LEG_OPAC_N); // Converter overlay owns its subtitle style.
  dtsSessao = NULL;
  midia[0] = 0;
  temEstilo = ligado = 0;
  lsCall = NULL;
  bus = NULL;
  puts("PASS: all 20 subtitle opacity levels reach the native webOS player");
}

int main(void) {
  test_native_subtitle_opacity();
  dtsSessao=NULL;posSeg=10;durSeg=100;bufferSeg=20;
  movie_snapshot=(DtsBufferInfo){.ready_bytes=200,.capacity_bytes=256,.total=1000};
  movie_active=1;
  assert(fabs(video_buffer_fim()-40)<.001);
  movie_snapshot.ready_bytes=950;
  assert(fabs(video_buffer_fim()-100)<.001);
  movie_snapshot.ready_bytes=0;
  assert(video_buffer_fim()==20);
  movie_snapshot.ready_bytes=200;movie_active=-1;
  assert(video_buffer_fim()==20);
  movie_active=1;dtsSessao=(DtsPlayback *)(uintptr_t)1;
  assert(video_buffer_fim()==20); // Inactive converter ignores the native proxy.
  pump_status=(DtsPlaybackStatus){.prepared=1,
    .buffer={.ready_bytes=12*1048576LL,.capacity_bytes=132*1048576LL,
             .total=100*1048576LL,.contiguous_bytes=6*1048576LL},.buffered_until=14};
  bufferSeg=0;
  assert(fabs(video_buffer_fim()-16)<.001); // Stop at a gap, not all 12 MiB ready.
  pump_status.buffered_until=25;
  assert(fabs(video_buffer_fim()-25)<.001); // Include frames already fed to LG.
  pump_status.buffer.contiguous_bytes=100*1048576LL;
  assert(video_buffer_fim()==100); // Never extend past the file duration.
  pump_status.failed=1;
  assert(video_buffer_fim()==0);
  pump_status=(DtsPlaybackStatus){0};dtsSessao=NULL;
  movie_active=1;sessao=41;ligado=0;midia[0]=0;acb=0;
  int stops=movie_stops;
  pararSessao();
  assert(movie_stops==stops+1 && !movie_active && movie_stop_session==42);
  puts("video_moviebuffer: native/DTS rail, range gaps, duration clamp, fallback and shutdown passed");
}
