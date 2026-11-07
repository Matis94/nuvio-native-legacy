/* External audio (dts_engine_external_audio): the video of one file with the
 * audio of another, interleaved, shifted by the offset and trimmed on seek.
 * Local files stand in for both HTTP transports: ranges for the main source,
 * the single streaming read for the second one. */
#include "../src/dts/dts_engine.h"
#include <assert.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char *HEADERS = "X-Fixture: yes";
static unsigned streams_opened;

char *rede_range_headers(const char *raw, int public_only) {
  (void)public_only; return strdup(raw ? raw : "");
}
int rede_fluxo64(const char *url, const char *headers, int64_t start,
                 int (*sink)(void *, const char *, size_t), void *opaque,
                 int64_t *total, int *status, volatile int *cancel,
                 char *final, int *cross, long stall_ms, int64_t *bytes) {
  (void)stall_ms;
  assert(!strcmp(headers, HEADERS));
  if (final) final[0] = 0;
  if (cross) *cross = 0;
  *bytes = 0; *status = 0;
  FILE *f = fopen(url, "rb");
  if (!f) return -1;
  __atomic_add_fetch(&streams_opened, 1, __ATOMIC_RELAXED);
  fseeko(f, 0, SEEK_END); *total = ftello(f); fseeko(f, start, SEEK_SET);
  *status = 206;
  char buf[65536];
  for (;;) {
    if (__atomic_load_n(cancel, __ATOMIC_RELAXED)) { fclose(f); return -1; }
    size_t n = fread(buf, 1, sizeof buf, f);
    if (!n) break;
    if (!sink(opaque, buf, n)) { fclose(f); return 1; }
    *bytes += (int64_t)n;
  }
  fclose(f);
  return 0;
}
char *rede_baixar_trecho64_cab(const char *url, const char *headers, int64_t start, int64_t end,
                               long *size, int64_t *total, int *status, volatile int *cancelled) {
  assert(!strcmp(headers, HEADERS));
  *size = 0; *status = 0; *total = -1;
  if (__atomic_load_n(cancelled, __ATOMIC_RELAXED)) return NULL;
  FILE *f = fopen(url, "rb");
  if (!f) return NULL;
  fseeko(f, 0, SEEK_END); *total = ftello(f);
  if (start >= *total) { *status = 416; fclose(f); return NULL; }
  fseeko(f, start, SEEK_SET);
  long n = (long)(end - start + 1);
  if (n > *total - start) n = (long)(*total - start);
  char *data = malloc((size_t)n); assert(data);
  *size = (long)fread(data, 1, (size_t)n, f); fclose(f); *status = 206;
  return data;
}
char *rede_baixar_trecho64_final(const char *url, const char *headers, int64_t start, int64_t end,
                                 long *size, int64_t *total, int *status, volatile int *cancelled,
                                 char *final, int *cross) {
  if (final) final[0] = 0;
  if (cross) *cross = 0;
  return rede_baixar_trecho64_cab(url, headers, start, end, size, total, status, cancelled);
}
char *rede_baixar_trecho64_vigiado(const char *u, const char *h, int64_t s, int64_t e, long *n,
                                   int64_t *t, int *status, volatile int *c, char *f, int *x,
                                   volatile int *urgent, unsigned idle, int *stalled) {
  (void)urgent; (void)idle; if (stalled) *stalled = 0;
  return rede_baixar_trecho64_final(u, h, s, e, n, t, status, c, f, x);
}
char *rede_baixar_trecho64_relato(const char *u, const char *h, int64_t s, int64_t e, long *n,
                                  int64_t *t, int *status, volatile int *c, char *f, int *x,
                                  volatile int *urgent, unsigned idle, int *stalled, int *retry_after) {
  (void)urgent; (void)idle; if (stalled) *stalled = 0; if (retry_after) *retry_after = -1;
  return rede_baixar_trecho64_final(u, h, s, e, n, t, status, c, f, x);
}

/* The app passes ling_casa; here "pl" also matches "pol". */
static int lang_match(const char *code, const char *pref) {
  if (!strcasecmp(code, pref)) return 1;
  return !strcasecmp(pref, "pl") && !strcasecmp(code, "pol");
}

/* Packets of the chosen stream in the other file, to check bytes and times. */
typedef struct { int64_t pts_ns; int size; unsigned sum; } Ref;
static Ref refs[4096];
static int nrefs;
static unsigned checksum(const unsigned char *d, size_t n) {
  unsigned s = 2166136261u;
  for (size_t i = 0; i < n; i++) s = (s ^ d[i]) * 16777619u;
  return s;
}
/* The host FFmpeg has no file protocol: read through a stdio AVIO. */
static int file_read(void *o, uint8_t *b, int n) {
  size_t k = fread(b, 1, (size_t)n, o);
  return k ? (int)k : AVERROR_EOF;
}
static int64_t file_seek(void *o, int64_t off, int whence) {
  FILE *f = o;
  if (whence == AVSEEK_SIZE) { int64_t at = ftello(f), end; fseeko(f, 0, SEEK_END); end = ftello(f); fseeko(f, at, SEEK_SET); return end; }
  return fseeko(f, off, whence & ~AVSEEK_FORCE) ? -1 : ftello(f);
}
static void load_refs(const char *path, const char *lang) {
  FILE *file = fopen(path, "rb");
  assert(file);
  AVFormatContext *fmt = avformat_alloc_context();
  AVIOContext *io = avio_alloc_context(av_malloc(32768), 32768, 0, file, file_read, NULL, file_seek);
  assert(fmt && io);
  fmt->pb = io; fmt->flags |= AVFMT_FLAG_CUSTOM_IO;
  assert(avformat_open_input(&fmt, NULL, NULL, NULL) == 0);
  assert(avformat_find_stream_info(fmt, NULL) >= 0);
  int k = -1;
  for (unsigned i = 0; i < fmt->nb_streams; i++) {
    AVDictionaryEntry *t = av_dict_get(fmt->streams[i]->metadata, "language", NULL, 0);
    if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && t && !strcmp(t->value, lang)) k = (int)i;
  }
  assert(k >= 0);
  AVPacket *p = av_packet_alloc(); nrefs = 0;
  while (av_read_frame(fmt, p) >= 0) {
    if (p->stream_index == k && nrefs < 4096) {
      AVRational tb = fmt->streams[k]->time_base;
      refs[nrefs++] = (Ref){av_rescale_q(p->pts, tb, (AVRational){1, 1000000000}), p->size,
                            checksum(p->data, (size_t)p->size)};
    }
    av_packet_unref(p);
  }
  av_packet_free(&p); avformat_close_input(&fmt);
  av_freep(&io->buffer); avio_context_free(&io); fclose(file);
  assert(nrefs > 50);
}
static const Ref *ref_at(int64_t pts_ns) {
  for (int i = 0; i < nrefs; i++)
    if (llabs(refs[i].pts_ns - pts_ns) < 1000000) return &refs[i];
  return NULL;
}

typedef struct { int audio, video, subs; int64_t first_audio, last_audio, max_video, worst_lead; } Run;
/* Reads to the end; every external packet must match the other file. */
static int checkBytes = 1;
static Run play(DtsEngine *e, int64_t offset_ns, int limit) {
  Run r = {0, 0, 0, INT64_MIN, INT64_MIN, INT64_MIN, INT64_MIN};
  DtsFrame f;
  int got;
  while ((got = dts_engine_next(e, &f)) == 1 && (limit <= 0 || r.audio + r.video < limit)) {
    if (f.kind == DTS_VIDEO) {
      r.video++;
      if (f.pts_ns > r.max_video) r.max_video = f.pts_ns;
    } else if (f.kind == DTS_AUDIO) {
      assert(f.stream_index == DTS_ENGINE_EXTERNAL_STREAM);
      assert(r.max_video != INT64_MIN);            /* never before the first picture */
      int64_t lead = f.pts_ns - r.max_video;
      if (lead > r.worst_lead) r.worst_lead = lead;
      assert(r.last_audio == INT64_MIN || f.pts_ns > r.last_audio);
      if (checkBytes) {
        const Ref *ref = ref_at(f.pts_ns - offset_ns);
        assert(ref && ref->size == (int)f.size && ref->sum == checksum(f.data, f.size));
      }
      if (r.first_audio == INT64_MIN) r.first_audio = f.pts_ns;
      r.last_audio = f.pts_ns; r.audio++;
    } else r.subs++;
  }
  if (got < 0) { fprintf(stderr, "next: %s\n", dts_engine_error(e)); exit(1); }
  return r;
}

int main(int argc, char **argv) {
  assert(argc == 7);
  const char *main_file = argv[1], *other = argv[2], *dts_only = argv[3], *short_clip = argv[4], *mp4 = argv[5];
  const char *aac44 = argv[6];
  dts_engine_set_lang_match(lang_match);
  load_refs(other, "pol");

  /* 1. Preferred language, main audio hidden, 8 MiB ring for the other file. */
  DtsEngineRings rings;
  dts_engine_rings(&rings, 1);
  DtsEngine *e = dts_engine_create();
  dts_engine_external_audio(e, other, HEADERS, "de,pl", 0);
  if (dts_engine_open(e, main_file, HEADERS, -1, 0, 0) < 0) {
    fprintf(stderr, "open: %s\n", dts_engine_error(e)); return 1;
  }
  const DtsMediaInfo *info = dts_engine_info(e);
  assert(info->audio_stream == DTS_ENGINE_EXTERNAL_STREAM);
  assert(!strcmp(info->audio_codec, "ac3") && info->channels == 6 && info->sample_rate == 48000);
  /* Every audio track of the other version is listed for the menu: the one
   * playing as DTS_ENGINE_EXTERNAL_STREAM, the English one as + 1 + index. */
  int audio_tracks = 0, playing = 0, other_eng = 0;
  for (int i = 0; i < info->n_tracks; i++)
    if (info->tracks[i].kind == DTS_AUDIO) {
      audio_tracks++;
      if (info->tracks[i].stream_index == DTS_ENGINE_EXTERNAL_STREAM)
        playing += !strcmp(info->tracks[i].language, "pol");
      else other_eng += info->tracks[i].stream_index == DTS_ENGINE_EXTERNAL_STREAM + 1 + 1 &&
                        !strcmp(info->tracks[i].language, "eng") && !strcmp(info->tracks[i].codec, "eac3");
    }
  assert(audio_tracks == 2 && playing == 1 && other_eng == 1);
  Run r = play(e, 0, 0);
  assert(r.video == 100 && r.subs >= 1);
  assert(r.first_audio >= 0 && r.first_audio < 40000000);
  assert(r.last_audio > 3900000000LL);
  assert(r.worst_lead <= 500000000LL);
  dts_engine_rings(&rings, 0);
  assert(rings.peak == 1 && rings.peak_bytes == 8LL * 1024 * 1024);
  printf("plain: %d video, %d audio, worst lead %.0f ms, ring peak %lld MiB\n", r.video, r.audio,
         r.worst_lead / 1e6, rings.peak_bytes >> 20);

  /* 2. Seek with a negative offset (the other version runs ahead). */
  dts_engine_external_offset(e, -500);
  assert(dts_engine_seek(e, 2.0) == 0);
  r = play(e, -500000000LL, 0);
  assert(r.first_audio >= 1950000000LL && r.first_audio < 2050000000LL);
  assert(r.last_audio <= 4000000000LL);
  printf("seek 2.0 s, offset -500 ms: first audio at %.3f s\n", r.first_audio / 1e9);

  /* 3. A track change of the main file is refused while external is on. */
  assert(dts_engine_audio(e, 1, 0, 0) < 0);
  dts_engine_destroy(e);

  /* 4. Positive offset from the start of playback, open at 1.0 s. */
  e = dts_engine_create();
  dts_engine_external_audio(e, other, HEADERS, "pl", 1000);
  if (dts_engine_open(e, main_file, HEADERS, -1, 0, 1.0) < 0) {
    fprintf(stderr, "open: %s\n", dts_engine_error(e)); return 1;
  }
  r = play(e, 1000000000LL, 0);
  assert(r.first_audio >= 990000000LL && r.first_audio < 1100000000LL);
  printf("open at 1.0 s, offset +1000 ms: first audio at %.3f s\n", r.first_audio / 1e9);
  dts_engine_destroy(e);

  /* 5. No preferred language present: the first usable track (E-AC-3, eng). */
  e = dts_engine_create();
  dts_engine_external_audio(e, other, HEADERS, "fr", 0);
  assert(dts_engine_open(e, main_file, HEADERS, -1, 0, 0) == 0);
  assert(!strcmp(dts_engine_info(e)->audio_codec, "eac3"));
  dts_engine_destroy(e);

  /* 6. Only DTS in the other file: decoded and re-encoded, stereo AAC by
   * default and AC-3 with the 5.1 setting; seek and offset still apply. */
  checkBytes = 0;
  for (int ac3 = 0; ac3 < 2; ac3++) {
    dts_engine_set_ac3(ac3);
    e = dts_engine_create();
    dts_engine_external_audio(e, dts_only, HEADERS, "", 250);
    if (dts_engine_open(e, main_file, HEADERS, -1, 0, 0) < 0) { fprintf(stderr, "open: %s\n", dts_engine_error(e)); return 1; }
    assert(!strcmp(dts_engine_info(e)->audio_codec, ac3 ? "ac3" : "aac"));
    assert(dts_engine_info(e)->sample_rate == 48000);
    int dtsTrack = 0;
    for (int i = 0; i < dts_engine_info(e)->n_tracks; i++)
      if (dts_engine_info(e)->tracks[i].kind == DTS_AUDIO) { dtsTrack = !strcmp(dts_engine_info(e)->tracks[i].codec, "dts"); }
    assert(dtsTrack);   /* the menu shows the source codec */
    r = play(e, 250000000LL, 0);
    assert(r.video == 100 && r.audio > 100);
    assert(r.first_audio >= 200000000LL && r.first_audio < 330000000LL);
    assert(r.last_audio > 3800000000LL && r.worst_lead <= 500000000LL);
    assert(dts_engine_seek(e, 2.0) == 0);
    r = play(e, 250000000LL, 0);
    assert(r.first_audio >= 1950000000LL && r.first_audio < 2100000000LL);
    printf("DTS donor -> %s: %d audio packets, seek 2.0 s first at %.3f s\n", ac3 ? "AC-3" : "AAC", r.audio, r.first_audio / 1e9);
    dts_engine_destroy(e);
  }
  dts_engine_set_ac3(0);
  /* DTS conversion off in Settings: the DTS donor is refused. */
  dts_engine_set_dts_off(1);
  e = dts_engine_create();
  dts_engine_external_audio(e, dts_only, HEADERS, "", 0);
  assert(dts_engine_open(e, main_file, HEADERS, -1, 0, 0) < 0);
  assert(strstr(dts_engine_error(e), "External audio: no audio track"));
  dts_engine_destroy(e);
  dts_engine_set_dts_off(0);
  checkBytes = 1;

  /* 7. A much shorter other file (a debrid "not cached" clip) is refused. */
  e = dts_engine_create();
  dts_engine_external_audio(e, short_clip, HEADERS, "", 0);
  assert(dts_engine_open(e, main_file, HEADERS, -1, 0, 0) < 0);
  assert(strstr(dts_engine_error(e), "not cached"));
  dts_engine_destroy(e);

  /* 8. Audio only: an MP4 donor fetches its audio chunks, not its video. */
  {
    int64_t total = 0, bytes;
    load_refs(mp4, "pol");
    for (int only = 0; only < 2; only++) {
      e = dts_engine_create();
      dts_engine_external_audio(e, mp4, HEADERS, "pl", 0);
      dts_engine_external_audio_only(e, only);
      if (dts_engine_open(e, main_file, HEADERS, -1, 0, 0) < 0) { fprintf(stderr, "open: %s\n", dts_engine_error(e)); return 1; }
      r = play(e, 0, 0);
      assert(r.video == 100 && r.audio > 100 && r.last_audio > 3800000000LL);
      bytes = dts_engine_external_bytes(e, &total);
      printf("MP4 donor %s: %lld of %lld bytes downloaded\n", only ? "audio only" : "whole file", (long long)bytes, (long long)total); fflush(stdout);
      /* The header read at open fills the donor's 8 MiB ring once (nothing
       * next to a real file); after it only the audio chunks are fetched, so
       * the 25 MiB donor stays well under half. */
      if (only) assert(bytes < total / 2 && bytes < (int64_t)(DTS_ENGINE_EXTERNAL_RING_MIB + 2) * 1048576);
      /* A seek back reads the earlier chunks again. */
      assert(dts_engine_seek(e, 1.0) == 0);
      r = play(e, 0, 0);
      assert(r.first_audio >= 950000000LL && r.first_audio < 1100000000LL);
      dts_engine_destroy(e);
    }
    /* Matroska has no audio index: audio only reads the whole file. */
    load_refs(other, "pol");
    e = dts_engine_create();
    dts_engine_external_audio(e, other, HEADERS, "pl", 0);
    dts_engine_external_audio_only(e, 1);
    assert(dts_engine_open(e, main_file, HEADERS, -1, 0, 0) == 0);
    r = play(e, 0, 0);
    assert(r.audio > 100);
    dts_engine_destroy(e);
  }

  /* 9. AAC at 44.1 kHz (a common WEB-DL dub) is converted, where the build
   * has the AAC decoder (the TV and CI builds do). */
  if (avcodec_find_decoder(AV_CODEC_ID_AAC)) {
    checkBytes = 0;
    e = dts_engine_create();
    dts_engine_external_audio(e, aac44, HEADERS, "pl", 0);
    if (dts_engine_open(e, main_file, HEADERS, -1, 0, 0) < 0) { fprintf(stderr, "open: %s\n", dts_engine_error(e)); return 1; }
    assert(!strcmp(dts_engine_info(e)->audio_codec, "aac") && dts_engine_info(e)->sample_rate == 48000);
    r = play(e, 0, 0);
    assert(r.audio > 100 && r.last_audio > 3800000000LL);
    printf("AAC 44.1 kHz donor converted: %d packets\n", r.audio);
    dts_engine_destroy(e);
    checkBytes = 1;
  } else printf("AAC decoder not in this FFmpeg build: 44.1 kHz case skipped\n");

  /* 10. Turning it off restores the main file's own track selection. */
  e = dts_engine_create();
  dts_engine_external_audio(e, other, HEADERS, "pl", 0);
  dts_engine_external_audio(e, NULL, NULL, NULL, 0);
  assert(!dts_engine_external_active(e));
  assert(dts_engine_open(e, main_file, HEADERS, -1, 0, 0) == 0);
  assert(dts_engine_info(e)->audio_stream == 1);
  dts_engine_destroy(e);

  dts_engine_rings(&rings, 0);
  assert(rings.alive == 0 && rings.bytes == 0);
  assert(streams_opened >= 4);
  /* Track picked in the menu: the English E-AC-3 (stream 1) over the
   * preferred Polish one. */
  e = dts_engine_create();
  dts_engine_external_audio(e, other, HEADERS, "pl", 0);
  dts_engine_external_track(e, 1);
  if (dts_engine_open(e, main_file, HEADERS, -1, 0, 0) < 0) { fprintf(stderr, "open: %s\n", dts_engine_error(e)); return 1; }
  info = dts_engine_info(e);
  assert(!strcmp(info->audio_codec, "eac3") && info->channels == 2);
  load_refs(other, "eng");   /* the packets are the English track's, byte for byte */
  r = play(e, 0, 0);
  assert(r.video == 100 && r.audio > 100);
  dts_engine_destroy(e);
  printf("External audio engine checks passed.\n");
  return 0;
}
