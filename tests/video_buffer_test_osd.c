/* Exercise the actual webOS reporter with the existing native bus fixture. */
#define NV_HTTP_BUFFER_DIAG 1
#define main video_acb_fixture_main
#include "video_acb_lg.c"
#undef main
int main(void) {
  char text[192];
  test_buffer_preset = 3;
  movie_active = 1;
  movie_snapshot = (DtsBufferInfo){.ready_bytes=84*1048576LL+524288,
                                  .capacity_bytes=128*1048576LL};
  video_buffer_test_text(text, sizeof text);
  assert(!strcmp(text, "Buffer: 84.5 / 128 MiB total (256 MiB selected)"));
  for (int cap = 64; cap <= 256; cap *= 2) {
    movie_snapshot.capacity_bytes = (int64_t)cap * 1048576;
    movie_snapshot.ready_bytes = movie_snapshot.capacity_bytes;
    video_buffer_test_text(text, sizeof text);
    char expected[192];
    snprintf(expected, sizeof expected, "Buffer: %.1f / %d MiB total%s", (double)cap, cap,
             cap == 256 ? "" : " (256 MiB selected)");
    assert(!strcmp(text, expected));
  }
  movie_snapshot = (DtsBufferInfo){0};
  video_buffer_test_text(text, sizeof text);assert(!strcmp(text, "Buffer: system"));
  movie_snapshot = (DtsBufferInfo){.ready_bytes=20*1048576LL,.capacity_bytes=64*1048576LL};
  movie_active = -1;
  video_buffer_test_text(text, sizeof text);assert(!strcmp(text, "Buffer: system"));
  movie_active = 0;
  video_buffer_test_text(text, sizeof text);assert(!strcmp(text, "Buffer: system"));
  movie_active = 1;dtsSessao = (DtsPlayback *)(uintptr_t)1;
  video_buffer_test_text(text, sizeof text);assert(!strcmp(text, "Buffer: system"));
  dtsSessao = NULL;test_buffer_preset = 1;
  video_buffer_test_text(text, sizeof text);assert(!strcmp(text, "Buffer: 20.0 / 64 MiB total"));
  movie_snapshot.ready_bytes = -100;
  video_buffer_test_text(text, sizeof text);assert(!strcmp(text, "Buffer: 0.0 / 64 MiB total"));
  movie_snapshot.ready_bytes = 100*1048576LL;
  video_buffer_test_text(text, sizeof text);assert(!strcmp(text, "Buffer: 64.0 / 64 MiB total"));
  movie_active = 0;
  char tiny[2] = {1,1};video_buffer_test_text(tiny, sizeof tiny);assert(tiny[1] == 0);
  video_buffer_test_text(NULL, 0);
  ligado = 0;midia[0] = 0;acb = 0;pararSessao();
  puts("video_buffer_test_osd: measured capacity, reduced budget, native unknown, fallback, converter isolation and reset passed");
}
