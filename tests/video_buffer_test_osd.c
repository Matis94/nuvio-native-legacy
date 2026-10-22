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
  assert(!strcmp(text, "Buffer RAM: 84.5 / 128 MiB | setting: 256 MiB"));
  for (int cap = 64; cap <= 256; cap *= 2) {
    movie_snapshot.capacity_bytes = (int64_t)cap * 1048576;
    movie_snapshot.ready_bytes = movie_snapshot.capacity_bytes;
    video_buffer_test_text(text, sizeof text);
    char expected[192];
    snprintf(expected, sizeof expected, "Buffer RAM: %.1f / %d MiB | setting: 256 MiB", (double)cap, cap);
    assert(!strcmp(text, expected));
  }
  movie_snapshot = (DtsBufferInfo){0};
  video_buffer_test_text(text, sizeof text);
  assert(strstr(text, "allocation pending") && !strstr(text, "/ 256"));
  movie_active = -1;bufferTestNativeMiB = 256;
  video_buffer_test_text(text, sizeof text);
  assert(strstr(text, "webOS fallback") && strstr(text, "actual size unknown") && !strstr(text, "requested"));
  movie_active = 0;bufferTestNativeMiB = 128;
  video_buffer_test_text(text, sizeof text);
  assert(strstr(text, "requested: 128 MiB") && strstr(text, "actual size unknown"));
  bufferTestNativeMiB = 0;
  video_buffer_test_text(text, sizeof text);
  assert(strstr(text, "webOS Auto") && strstr(text, "setting: 256 MiB"));
  movie_active = 1;dtsSessao = (DtsPlayback *)(uintptr_t)1;
  video_buffer_test_text(text, sizeof text);
  assert(strstr(text, "DTS/DV path") && strstr(text, "actual size unknown"));
  dtsSessao = NULL;movie_active = 0;test_buffer_preset = 0;
  video_buffer_test_text(text, sizeof text);
  assert(strstr(text, "setting: Auto"));
  char tiny[2] = {1,1};video_buffer_test_text(tiny, sizeof tiny);assert(tiny[1] == 0);
  video_buffer_test_text(NULL, 0);
  bufferTestNativeMiB = 256;ligado = 0;midia[0] = 0;acb = 0;
  pararSessao();assert(!bufferTestNativeMiB);
  puts("video_buffer_test_osd: measured capacity, reduced budget, native unknown, fallback, converter isolation and reset passed");
}
