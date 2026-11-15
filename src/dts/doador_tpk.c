// AUDIO FROM ANOTHER SOURCE on the Samsung .tpk (donor, owner 15/11): see
// doador_tpk.h. The server is the one of TorBox burn-in Passthrough (branch
// subtitle-rewrite, src/dts/tbaudio_tpk.c), without TorBox: the video is the
// local DTS converter's in video-only mode, the audio renditions come from the
// playing file and from a donor file.
#include "doador_tpk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>

#if !defined(NV_DTS_FFMPEG) || !(defined(NV_TPK) || defined(NV_DOADOR_TEST))
int  doa_disponivel(void) { return 0; }
int  doa_abrir(const DoaAbrir *o, char *out, size_t n) { (void)o; if (out && n) out[0] = 0; return 0; }
void doa_fechar(void) {}
int  doa_ativo(void) { return 0; }
int  doa_pronto(void) { return 0; }
int  doa_falhou(char *m, size_t n) { if (m && n) m[0] = 0; return 0; }
int  doa_rendicoes(DoaRendicao *out, int max) { (void)out; (void)max; return 0; }
int  doa_rendicao_de(int chave) { (void)chave; return -1; }
void doa_definir_audio(int chave) { (void)chave; }
int  doa_audio(void) { return -1; }
void doa_desloc(int ms) { (void)ms; }
int  doa_desloc_ms(void) { return 0; }
int  doa_estado(char *m, size_t n) { if (m && n) m[0] = 0; return 0; }
void doa_log(void) {}
void doa_definir_casa(int (*casa)(const char *, const char *)) { (void)casa; }
int  doa_diferenca(int *c, int *v, int *p) { (void)c; (void)v; (void)p; return 0; }
int  doa_sondar(const char *u, const char *c, DoaSonda *o, int m) { (void)u; (void)c; (void)o; (void)m; return -1; }
void doa_dopasuj(int a) { (void)a; }
int  doa_dopasuj_resultado(DoaDopasuj *o, int *a) { (void)o; (void)a; return 0; }
int  doa_dopasuj_ativo(void) { return 0; }
int  doa_doador_saude(int *l) { if (l) *l = 0; return 0; }
int  doa_seguir_evento(int *ms) { (void)ms; return 0; }
#else

#include "dts_engine.h"
#include "dts_remux.h"
#include "dts_match.inc"
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#define TBA_FAIXAS     32
#define TBA_FONTES     2                 // the playing file, and one donor
#define TBA_CONEXOES   24                // the player may open every rendition at once
#define ESPERA_PREP_MS 30000
#define FILA_MAX       512
#define AUD_SEG_S      2.0               // audio segments: a seek reads ~2 s
// A donor that does not open by then is dropped: the reader retries a dead
// link for ~30 s (expired, not cached), and the player waits for the session.
#define DOADOR_PRAZO_MS 10000

// ---- S2: connections to the files' hosts in /proc/net/tcp (logging only) ----
#define SK_IPS 24
static pthread_mutex_t skTrava = PTHREAD_MUTEX_INITIALIZER;
static unsigned skIp[SK_IPS];
static int skN = -1;
static char *skUrl[4];
static void skHost(const char *u, char *h, size_t n) {
  const char *p = u ? strstr(u, "://") : NULL;
  size_t k;
  h[0] = 0;
  if (!p) return;
  p += 3; k = strcspn(p, "/?#:");
  { const char *at = memchr(p, '@', k); if (at) { k -= (size_t)(at + 1 - p); p = at + 1; } }
  snprintf(h, n, "%.*s", (int)k, p);
}
static void *skResolver(void *u) {
  unsigned ip[SK_IPS];
  char hosts[4][256];
  int n = 0, i, j;
  (void)u;
  pthread_mutex_lock(&skTrava);
  for (i = 0; i < 4; i++) { skHost(skUrl[i], hosts[i], sizeof hosts[i]); free(skUrl[i]); skUrl[i] = NULL; }
  pthread_mutex_unlock(&skTrava);
  for (i = 0; i < 4; i++) {
    struct addrinfo dica, *r = NULL, *a;
    if (!hosts[i][0] || !strcmp(hosts[i], "127.0.0.1")) continue;
    for (j = 0; j < i; j++) if (!strcmp(hosts[j], hosts[i])) break;
    if (j < i) continue;
    memset(&dica, 0, sizeof dica); dica.ai_family = AF_INET; dica.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(hosts[i], NULL, &dica, &r) != 0) continue;
    for (a = r; a && n < SK_IPS; a = a->ai_next) {
      unsigned v = ((struct sockaddr_in *)a->ai_addr)->sin_addr.s_addr;
      for (j = 0; j < n; j++) if (ip[j] == v) break;
      if (j == n) ip[n++] = v;
    }
    freeaddrinfo(r);
  }
  pthread_mutex_lock(&skTrava);
  memcpy(skIp, ip, sizeof ip); skN = n;
  pthread_mutex_unlock(&skTrava);
  printf("[doador] sockets: watching %d address(es) of the files' hosts\n", n);
  fflush(stdout);
  return NULL;
}
static void skVigiar(const char *u1, const char *u2, const char *u3, const char *u4) {
  const char *u[4] = { u1, u2, u3, u4 };
  pthread_t t;
  int i;
  pthread_mutex_lock(&skTrava);
  for (i = 0; i < 4; i++) { free(skUrl[i]); skUrl[i] = u[i] && u[i][0] ? strdup(u[i]) : NULL; }
  skN = -1;
  pthread_mutex_unlock(&skTrava);
  if (pthread_create(&t, NULL, skResolver, NULL) == 0) pthread_detach(t);
}
// Inodes of this process's sockets (/proc/self/fd -> "socket:[N]").
static int skMeus(unsigned long *v, int max) {
  DIR *d = opendir("/proc/self/fd");
  struct dirent *e;
  int n = 0;
  if (!d) return -1;
  while ((e = readdir(d)) != NULL && n < max) {
    char c[300], alvo[64];
    ssize_t k;
    snprintf(c, sizeof c, "/proc/self/fd/%s", e->d_name);
    k = readlink(c, alvo, sizeof alvo - 1);
    if (k <= 0) continue;
    alvo[k] = 0;
    if (!strncmp(alvo, "socket:[", 8)) v[n++] = strtoul(alvo + 8, NULL, 10);
  }
  closedir(d);
  return n;
}
static int skArquivo(const char *nome, const unsigned *ip, int nIp, const unsigned long *meus, int nMeus, int *eu) {
  FILE *f = fopen(nome, "r");
  char l[512];
  int tot = 0;
  if (!f) return -1;
  if (!fgets(l, sizeof l, f)) { fclose(f); return 0; }
  while (fgets(l, sizeof l, f)) {
    char rem[64], st[8];
    unsigned long ino = 0;
    unsigned v = 0;
    int i, ok = 0;
    if (sscanf(l, "%*s %*s %63s %7s %*s %*s %*s %*s %*s %lu", rem, st, &ino) < 3) continue;
    if (strcmp(st, "01")) continue;   // ESTABLISHED
    if (strlen(rem) >= 13 && rem[8] == ':') v = (unsigned)strtoul(rem, NULL, 16);   // tcp
    else if (strlen(rem) >= 37 && rem[32] == ':' && !strncmp(rem, "0000000000000000", 16) &&
             !strncmp(rem + 16, "FFFF0000", 8)) {   // tcp6 ::ffff:a.b.c.d (words in host order)
      char w[9];
      memcpy(w, rem + 24, 8); w[8] = 0;
      v = (unsigned)strtoul(w, NULL, 16);
    } else continue;
    for (i = 0; i < nIp; i++) if (ip[i] == v) { ok = 1; break; }
    if (!ok) continue;
    tot++;
    for (i = 0; i < nMeus; i++) if (meus[i] == ino) { (*eu)++; break; }
  }
  fclose(f);
  return tot;
}
static int skContar(int *nuvio) {
  unsigned ip[SK_IPS];
  unsigned long meus[256];
  int n, nm, a, b, eu = 0;
  if (nuvio) *nuvio = -1;
  pthread_mutex_lock(&skTrava);
  n = skN; if (n > 0) memcpy(ip, skIp, sizeof ip);
  pthread_mutex_unlock(&skTrava);
  if (n <= 0) return -1;
  nm = skMeus(meus, 256);
  a = skArquivo("/proc/net/tcp", ip, n, meus, nm > 0 ? nm : 0, &eu);
  b = skArquivo("/proc/net/tcp6", ip, n, meus, nm > 0 ? nm : 0, &eu);
  if (a < 0 && b < 0) return -1;
  if (nuvio) *nuvio = nm >= 0 ? eu : -1;
  return (a > 0 ? a : 0) + (b > 0 ? b : 0);
}

enum { M_TS = 0, M_MP4, M_AC3, M_AAC, M_FORA };
static const char *const MODO_NOME[] = { "copied (MPEG-TS)", "copied (fMP4)", "to AC3", "to AAC", "not offered" };

typedef struct {
  int si, ord, canais, taxa, modo;
  int fonte, chave;                    // fo[] it comes from; the app's key (ord, or DOA_DOADOR(ord))
  enum AVCodecID id;
  char lingua[16], nome[96], codec[24];
} Faixa;

// An audio source: the playing file (fo[0]) or the donor (fo[1]).
typedef struct {
  char *url, *cab;
  char nome[96];                       // donor: the app's label for it ("" for the file itself)
  DtsEngine *eng;
  AVFormatContext *ic;
  double v0, dur, fim;                 // its video's start, its length, the container's end (s)
  double desloc;                       // its time * k + desloc = the playing file's time (donor: starts + offset)
  double k;                            // its speed against the file (1; a PAL version 25/23.976)
  int sBusca;                          // the stream seeks go through (the one the index covers)
  int leu;                             // the demuxer has read packets (a seek is needed for a fresh start)
  int conectada;                       // its reader holds a connection (0 after dts_engine_release)
  int falhou; char motivo[160];
  long nSoltas, nVoltas;               // connections given back, connections taken again
  double msVolta;                      // last reconnection: request to its segment out
} Fonte;

typedef struct { AVPacket *p; double t; } Item;   // t: the playing file's time (s) of the packet

// One side of the automatic match: its audio decoded to band envelopes.
typedef struct {
  AVCodecContext *dec; SwrContext *swr; AVFrame *fr;
  DtsMatchEnv ev; double t0, ult; int cap, temT0, si;
  float *env[DTS_MATCH_BANDS];
} Lado;

typedef struct Sessao {
  atomic_int refs;
  volatile int cancel;
  pthread_mutex_t m;                   // state below
  pthread_cond_t c;
  int pronto, falhou, flacConv, audio;   // audio: index in fx[]
  char motivo[160], tok[40];
  char *video;                         // the local converter's media playlist (absolute URL)
  Fonte fo[TBA_FONTES]; int nfo;
  int umaConexao;                      // one audio connection: the source not playing gives its connection back
  char doadorLingua[64];               // the donor track to start on: these languages in order (comma-separated)
  int doadorFaixa;                     // the donor stream picked by hand (+1; 0 none)
  char dicaLingua[16], dicaCodec[24], dicaTitulo[96]; int dicaCanais;   // series memory's track
  int difOk, difCorte, difVel, difPpm; // doa_diferenca
  double fpsRazao;                     // the donor's frame rate / the file's (0 unknown)
  int ppmPedido;                       // DoaAbrir.doador_ppm
  // automatic match (mm guards the sides while a tap feeds them)
  pthread_mutex_t mm;
  int mEstado, mAuto, mNovo;           // 0 idle, 1 running, 2 found, 3 not found; mNovo: a result not taken yet
  int mArmD;                           // the donor's generator feeds mDon
  Lado mArq, mDon;
  DtsEngine *mEng;                     // a wide search's own reader (cancelled on close)
  DoaDopasuj mRes;
  // following another cut
  int seguir, segAtivo, segFalhas, segPerdido, segEvento, segMs;
  int dFalhas, dSemResposta, dLento, dLentoDito;   // the donor's segments: failures in a row, slow ones
  double segProx;                      // the file's time of the next look (s)
  int doadorPadrao;                    // start on the donor's track (else on `audio`)
  int doadorMs;                        // the donor's offset the app set (ms, positive = later)
  int doadorFeito;                     // its preparation ended (the watchdog stops); under m
  int doadorVencido;                   // the watchdog cancelled its reader
  double dur;                          // the playlist's length: the file's container end
  double *aIni; int naSeg;             // audio segments (AUD_SEG_S)
  long banda; int largura, altura; long fps1000; char faixaVideo[8];   // the master's STREAM-INF
  Faixa fx[TBA_FAIXAS]; int nfx;
  // generator (gm)
  pthread_mutex_t gm;
  int gOk, gFaixa, gSeg, gModo, gFonte;  // gFonte: the source the generator read last (-1 none)
  double gDesloc;                      // that source's desloc when the generator positioned
  const double *gIni;                  // the segment list gSeg counts in
  atomic_int espera;                   // requests waiting for gm
  atomic_long espChave;                // the last one: (track+1) * 1000000 + segment
  Item fila[FILA_MAX]; int fIni, fN;
  AVPacket *pk;
  AVCodecContext *dec, *enc;
  SwrContext *swr;
  AVAudioFifo *fifo;
  AVFrame *fr, *ef;
  int64_t base48, amostras;
  int eof;
  // log
  long nSeg, nErro, nLarg; long long bytes; double msLer;
  char ultErro[160];
} Sessao;

static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static int (*casaLingua)(const char *, const char *);
void doa_definir_casa(int (*casa)(const char *, const char *)) { casaLingua = casa; }
static Sessao *atual;
static int ouvinte = -1, porta;
static atomic_int vivos;

static double agoraMs(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}
static void gerarToken(char *t, int n) {
  static const char A[] = "abcdefghjkmnpqrstuvwxyz23456789";
  unsigned char b[32];
  int f = open("/dev/urandom", O_RDONLY), k = 0, i;
  ssize_t r = f >= 0 ? read(f, b, sizeof b) : -1;
  if (f >= 0) close(f);
  if (r < (ssize_t)sizeof b) {
    uint64_t x = (uint64_t)agoraMs() * 6364136223846793005ULL + (uint64_t)getpid();
    for (i = 0; i < (int)sizeof b; i++) { x = x * 6364136223846793005ULL + 1442695040888963407ULL; b[i] = (unsigned char)(x >> 33); }
  }
  for (i = 0; i < (int)sizeof b && k < n - 1; i++) if (b[i] < 248) t[k++] = A[b[i] % 31];
  t[k] = 0;
}

static Sessao *pegar(void) {
  Sessao *s;
  pthread_mutex_lock(&trava);
  s = atual;
  if (s) atomic_fetch_add(&s->refs, 1);
  pthread_mutex_unlock(&trava);
  return s;
}
static void filaLimpar(Sessao *s) {
  while (s->fN) { av_packet_free(&s->fila[s->fIni].p); s->fIni = (s->fIni + 1) % FILA_MAX; s->fN--; }
  s->fIni = 0;
}
static void codecLiberar(Sessao *s) {
  avcodec_free_context(&s->dec); avcodec_free_context(&s->enc);
  swr_free(&s->swr); av_audio_fifo_free(s->fifo); s->fifo = NULL;
}
// ---- automatic match: the sides (as the webOS engine's MatchSide) ----------------
static void ladoLiberar(Lado *l) {
  int b;
  avcodec_free_context(&l->dec); swr_free(&l->swr); av_frame_free(&l->fr);
  for (b = 0; b < DTS_MATCH_BANDS; b++) av_freep(&l->env[b]);
  memset(l, 0, sizeof *l);
}
// Ready to take `cap` blocks of stream `si` (decoder opened on its first packet).
static int ladoPreparar(Lado *l, int si, int cap) {
  int b;
  ladoLiberar(l);
  l->si = si; l->cap = cap;
  for (b = 0; b < DTS_MATCH_BANDS; b++) if (!(l->env[b] = av_malloc(sizeof(float) * (size_t)cap))) return -1;
  return 0;
}
static int ladoCheio(const Lado *l) { return l->ev.max > 0 && l->ev.n >= l->ev.max; }
static void ladoSig(const Lado *l, DtsMatchSig *g) {
  int b;
  for (b = 0; b < DTS_MATCH_BANDS; b++) g->env[b] = l->ev.max ? l->ev.env[b] : NULL;
  g->n = l->ev.n; g->t0 = l->temT0 ? l->t0 : 0;
}
static void ladoQuadro(Lado *l, AVFrame *fr, AVRational tb) {
  float buf[8192];
  uint8_t *out = (uint8_t *)buf;
  int got;
  if (!l->swr) {
    AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
    if (swr_alloc_set_opts2(&l->swr, &mono, AV_SAMPLE_FMT_FLT, fr->sample_rate, &fr->ch_layout, fr->format,
                            fr->sample_rate, 0, NULL) < 0 || swr_init(l->swr) < 0) { swr_free(&l->swr); av_frame_unref(fr); return; }
    dts_match_env_init(&l->ev, fr->sample_rate, l->env, l->cap);
  }
  if (!l->temT0) {
    int64_t p = fr->best_effort_timestamp != AV_NOPTS_VALUE ? fr->best_effort_timestamp : fr->pts;
    if (p == AV_NOPTS_VALUE) { av_frame_unref(fr); return; }
    l->t0 = p * av_q2d(tb); l->temT0 = 1;
  }
  got = swr_convert(l->swr, &out, 8192, (const uint8_t **)fr->extended_data, fr->nb_samples);
  if (got > 0) dts_match_env_push(&l->ev, buf, got);
  av_frame_unref(fr);
}
// One packet of the side's stream. A jump in its time (a seek while it
// fills) starts the capture over from there. 1 full.
static int ladoPacote(Lado *l, const AVPacket *p, const AVStream *st) {
  double t;
  if (!l->cap || ladoCheio(l)) return 1;
  if (p->stream_index != l->si) return 0;
  if (!l->dec) {
    const AVCodec *c = avcodec_find_decoder(st->codecpar->codec_id);
    if (!c || !(l->dec = avcodec_alloc_context3(c)) || !(l->fr = av_frame_alloc())) { avcodec_free_context(&l->dec); return 0; }
    avcodec_parameters_to_context(l->dec, st->codecpar);
    l->dec->pkt_timebase = st->time_base;
    if (st->codecpar->codec_id == AV_CODEC_ID_DTS) av_opt_set_int(l->dec->priv_data, "core_only", 1, 0);
    if (avcodec_open2(l->dec, c, NULL) < 0) { avcodec_free_context(&l->dec); av_frame_free(&l->fr); return 0; }
  }
  t = p->pts != AV_NOPTS_VALUE ? p->pts * av_q2d(st->time_base) : l->ult;
  if (l->temT0 && (t < l->ult - 0.5 || t > l->ult + 3.0)) {
    int b;
    avcodec_flush_buffers(l->dec);
    l->ev.n = 0; l->ev.accn = 0; for (b = 0; b < DTS_MATCH_BANDS; b++) l->ev.acc[b] = 0;
    l->temT0 = 0;
  }
  l->ult = t;
  if (avcodec_send_packet(l->dec, p) < 0) return 0;
  while (avcodec_receive_frame(l->dec, l->fr) == 0) ladoQuadro(l, l->fr, st->time_base);
  return ladoCheio(l);
}

static void soltar(Sessao *s) {
  if (!s || atomic_fetch_sub(&s->refs, 1) != 1) return;
  filaLimpar(s); codecLiberar(s);
  av_packet_free(&s->pk); av_frame_free(&s->fr); av_frame_free(&s->ef);
  { int k; for (k = 0; k < TBA_FONTES; k++) {
      if (s->fo[k].eng) { dts_engine_cancel(s->fo[k].eng); dts_engine_destroy(s->fo[k].eng); }
      free(s->fo[k].url); free(s->fo[k].cab);
    } }
  free(s->aIni); free(s->video);
  ladoLiberar(&s->mArq); ladoLiberar(&s->mDon);
  pthread_mutex_destroy(&s->m); pthread_cond_destroy(&s->c); pthread_mutex_destroy(&s->gm); pthread_mutex_destroy(&s->mm);
  free(s);
}
static void falhar(Sessao *s, const char *porque) {
  pthread_mutex_lock(&s->m);
  if (!s->pronto && !s->falhou) {
    s->falhou = 1;
    snprintf(s->motivo, sizeof s->motivo, "%s", porque);
    printf("[doador] session failed: %s\n", porque);
    fflush(stdout);
  }
  pthread_cond_broadcast(&s->c);
  pthread_mutex_unlock(&s->m);
}

// ---- preparation ---------------------------------------------------------------
static const char *nomeLingua(const char *l) {
  static const struct { const char *c, *n; } T[] = {
    {"eng","English"},{"en","English"},{"jpn","Japanese"},{"ja","Japanese"},{"por","Portuguese"},{"pt","Portuguese"},
    {"spa","Spanish"},{"es","Spanish"},{"fre","French"},{"fra","French"},{"fr","French"},{"ger","German"},{"deu","German"},
    {"de","German"},{"ita","Italian"},{"it","Italian"},{"swe","Swedish"},{"sv","Swedish"},{"kor","Korean"},{"ko","Korean"},
    {"chi","Chinese"},{"zho","Chinese"},{"zh","Chinese"},{"rus","Russian"},{"ru","Russian"},{"hin","Hindi"},{"ara","Arabic"},
    {"pol","Polish"},{"pl","Polish"},{"ukr","Ukrainian"},{"uk","Ukrainian"},{"cze","Czech"},{"ces","Czech"},{"cs","Czech"},
    {"dut","Dutch"},{"nld","Dutch"},{"nl","Dutch"},{"tur","Turkish"},{"tr","Turkish"},{"hun","Hungarian"},{"hu","Hungarian"}};
  unsigned i;
  for (i = 0; i < sizeof T / sizeof T[0]; i++) if (!strcasecmp(l, T[i].c)) return T[i].n;
  return l[0] ? l : "Audio";
}
static int modoDe(const Faixa *f, int flacConv) {
  int ok = avcodec_find_decoder(f->id) != NULL;
  switch (f->id) {
    case AV_CODEC_ID_AAC: case AV_CODEC_ID_AC3: case AV_CODEC_ID_EAC3: case AV_CODEC_ID_OPUS:
    case AV_CODEC_ID_MP3: case AV_CODEC_ID_MP2: return M_TS;
    case AV_CODEC_ID_FLAC:
      if (!flacConv) return M_MP4;
      if (!ok) return M_FORA;
      return f->canais > 2 ? M_AC3 : M_AAC;
    case AV_CODEC_ID_DTS: case AV_CODEC_ID_TRUEHD: case AV_CODEC_ID_MLP: return ok ? M_AC3 : M_FORA;
    default: if (!ok) return M_FORA; return f->canais > 2 ? M_AC3 : M_AAC;
  }
}

// Opens audio source k (0 the playing file, 1 the donor), lists its audio
// tracks into fx[] and reads its seek index now (not on the viewer's first
// seek). 0 = failed (the reason in fo[k].motivo; for the file itself the
// session fails, for the donor only its tracks are missing).
static int prepFonte(Sessao *s, int k) {
  Fonte *fo = &s->fo[k];
  double t0 = agoraMs();
  AVFormatContext *ic;
  int i, v = -1, nA = 0, n0 = s->nfx;
  const char *quem = k ? "donor" : "original file";
  { DtsEngine *e = dts_engine_create();
    pthread_mutex_lock(&s->m);   // the donor's watchdog reads it
    fo->eng = e;
    if (e && k && s->doadorVencido) dts_engine_cancel(e);
    pthread_mutex_unlock(&s->m); }
  if (!fo->eng) { snprintf(fo->motivo, sizeof fo->motivo, "out of memory"); fo->falhou = 1; return 0; }
  if (s->cancel) return 0;
  if (dts_engine_open_demux(fo->eng, fo->url, fo->cab) < 0) {
    snprintf(fo->motivo, sizeof fo->motivo, "%s: %s", quem, dts_engine_error(fo->eng)); fo->falhou = 1; return 0;
  }
  fo->conectada = 1;
  ic = fo->ic = dts_engine_avformat(fo->eng);
  for (i = 0; i < (int)ic->nb_streams; i++) {
    AVStream *st = ic->streams[i];
    AVCodecParameters *p = st->codecpar;
    if (p->codec_type == AVMEDIA_TYPE_VIDEO && v < 0 && !(st->disposition & AV_DISPOSITION_ATTACHED_PIC)) v = i;
    if (p->codec_type == AVMEDIA_TYPE_AUDIO && s->nfx < TBA_FAIXAS) {
      Faixa *f = &s->fx[s->nfx++];
      AVDictionaryEntry *l = av_dict_get(st->metadata, "language", NULL, 0);
      AVDictionaryEntry *t = av_dict_get(st->metadata, "title", NULL, 0);
      f->si = i; f->ord = nA; f->id = p->codec_id; f->fonte = k;
      f->chave = k ? DOA_DOADOR(nA) : nA;
      f->canais = p->ch_layout.nb_channels; f->taxa = p->sample_rate;
      snprintf(f->lingua, sizeof f->lingua, "%s", l && strcmp(l->value, "und") ? l->value : "");
      snprintf(f->nome, sizeof f->nome, "%s", t ? t->value : "");
      snprintf(f->codec, sizeof f->codec, "%s", avcodec_get_name(p->codec_id));
    }
    if (p->codec_type == AVMEDIA_TYPE_AUDIO) nA++;
  }
  for (i = 0; i < (int)ic->nb_streams; i++) ic->streams[i]->discard = AVDISCARD_ALL;
  fo->dur = ic->duration > 0 ? ic->duration / (double)AV_TIME_BASE : 0;
  fo->fim = (ic->start_time != AV_NOPTS_VALUE ? ic->start_time / (double)AV_TIME_BASE : 0) + fo->dur;   // as dts_remux's endS
  fo->v0 = 0;
  if (v >= 0 && ic->streams[v]->start_time != AV_NOPTS_VALUE)
    fo->v0 = ic->streams[v]->start_time * av_q2d(ic->streams[v]->time_base);
  else if (ic->start_time != AV_NOPTS_VALUE) fo->v0 = ic->start_time / (double)AV_TIME_BASE;
  // The seek index (Matroska reads its Cues on the first seek): read now, not
  // on the viewer's first seek. Seeks then go through the stream the index
  // covers (the video): with stream -1 FFmpeg picks the non-discarded audio,
  // which has no index entries, and falls back to reading the file forward
  // (log 14/11: seeks of 7-13 s; on the TV's FFmpeg 7.1 a seek back failed).
  // The video's index wins whenever it has one; else the audio with the most
  // entries. NEVER a subtitle track: its Cues can outnumber the video's, but
  // they start at its first line and skip the gaps, and a seek before them
  // left the reader at mid-file (log 14/11, TTGA Kaguya S03E02: segment 0 got
  // no audio and the player waited forever).
  { double t1 = agoraMs();
    int r = -1, mais = -1;
    fo->sBusca = v >= 0 ? v : 0;
    if (fo->dur > 10)
      r = av_seek_frame(ic, fo->sBusca, av_rescale_q((int64_t)(fo->dur * 0.5 * AV_TIME_BASE), AV_TIME_BASE_Q,
                                                     ic->streams[fo->sBusca]->time_base), AVSEEK_FLAG_BACKWARD);
    if (v < 0 || avformat_index_get_entries_count(ic->streams[v]) <= 0)
      for (i = 0; i < (int)ic->nb_streams; i++) {
        int n = avformat_index_get_entries_count(ic->streams[i]);
        if (ic->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;
        if (n > mais) { mais = n; if (n > 0) fo->sBusca = i; }
      }
    fo->leu = 1;
    printf("[doador] %s: seek index %s in %.0f ms: seeks go through stream %d (%d entries)\n", quem, r >= 0 ? "read" : "NOT read",
           agoraMs() - t1, fo->sBusca, avformat_index_get_entries_count(ic->streams[fo->sBusca])); }
  for (i = n0; i < s->nfx; i++) s->fx[i].modo = modoDe(&s->fx[i], s->flacConv);
  printf("[doador] %s open in %.0f ms (%s): %.1f s, video starts at %.3f s, %d audio track(s)%s\n", quem,
         agoraMs() - t0, ic->iformat ? ic->iformat->name : "?", fo->dur, fo->v0, s->nfx - n0,
         k && v < 0 ? "; NO VIDEO: its start is the container's" : "");
  for (i = n0; i < s->nfx; i++)
    printf("[doador]   %s audio %d: %s %dch %d Hz, %s%s%s -> %s\n", k ? "donor" : "file", s->fx[i].ord, s->fx[i].codec,
           s->fx[i].canais, s->fx[i].taxa, s->fx[i].lingua[0] ? s->fx[i].lingua : "und", s->fx[i].nome[0] ? ", " : "",
           s->fx[i].nome, MODO_NOME[s->fx[i].modo]);
  fflush(stdout);
  return 1;
}

// The track the session starts on (the DEFAULT rendition): the donor's in the
// asked language (else its first offered track) when the donor leads, else
// the file's asked track, else its first offered one.
static int casa(const char *a, const char *b) {
  return a[0] && b[0] && (casaLingua ? casaLingua(a, b) : !strcasecmp(a, b));
}
static void escolherInicial(Sessao *s, int ordPedido) {
  int i;
  s->audio = -1;
  if (s->doadorPadrao && s->nfo > 1 && !s->fo[1].falhou) {
    // 1. The track picked by hand for this title.
    for (i = 0; i < s->nfx && s->audio < 0 && s->doadorFaixa > 0; i++)
      if (s->fx[i].fonte == 1 && s->fx[i].modo != M_FORA && s->fx[i].si == s->doadorFaixa - 1) s->audio = i;
    // 2. The series memory: in its language, the most like it.
    if (s->audio < 0 && s->dicaLingua[0]) {
      int melhor = -1;
      for (i = 0; i < s->nfx; i++) {
        const Faixa *f = &s->fx[i];
        int p;
        if (f->fonte != 1 || f->modo == M_FORA || !casa(f->lingua, s->dicaLingua)) continue;
        p = 1 + (s->dicaTitulo[0] && !strcasecmp(f->nome, s->dicaTitulo) ? 8 : 0) +
            (s->dicaCodec[0] && !strcmp(f->codec, s->dicaCodec) ? 4 : 0) + (s->dicaCanais && f->canais == s->dicaCanais ? 2 : 0);
        if (p > melhor) { melhor = p; s->audio = i; }
      }
    }
    // 3. The languages in order, then its first offered track.
    { const char *l = s->doadorLingua;
      while (s->audio < 0 && *l) {
        char um[16]; size_t k = strcspn(l, ",");
        snprintf(um, sizeof um, "%.*s", (int)(k < sizeof um ? k : sizeof um - 1), l);
        for (i = 0; i < s->nfx && s->audio < 0 && um[0]; i++)
          if (s->fx[i].fonte == 1 && s->fx[i].modo != M_FORA && casa(s->fx[i].lingua, um)) s->audio = i;
        l += k; if (*l == ',') l++;
      } }
    for (i = 0; i < s->nfx && s->audio < 0; i++) if (s->fx[i].fonte == 1 && s->fx[i].modo != M_FORA) s->audio = i;
  }
  for (i = 0; i < s->nfx && s->audio < 0; i++)
    if (s->fx[i].fonte == 0 && s->fx[i].chave == ordPedido && s->fx[i].modo != M_FORA) s->audio = i;
  for (i = 0; i < s->nfx && s->audio < 0; i++) if (s->fx[i].fonte == 0 && s->fx[i].modo != M_FORA) s->audio = i;
}

// One audio connection (owner 15/11, the TorBox budget): the source the
// generator does not read gives its connection back; its demuxer and index
// stay, so taking it again is one Range request.
static void fonteSoltar(Sessao *s, int k, const char *porque) {
  Fonte *fo = &s->fo[k];
  double t0 = agoraMs();
  if (!fo->eng || !fo->conectada) return;
  dts_engine_release(fo->eng);
  fo->conectada = 0; fo->nSoltas++;
  printf("[doador] %s gave its connection back in %.0f ms (%s)\n", k ? "donor" : "original file", agoraMs() - t0, porque);
  fflush(stdout);
}

// The converter's playlist starts at the file's time 0, and every packet it
// writes is its source time + DTS_REMUX_TS_OFFSET_S: the audio goes there too.
static double tl0(const Sessao *s) { (void)s; return 0.0; }
static double ptsDesloc(const Sessao *s) { (void)s; return (double)DTS_REMUX_TS_OFFSET_S; }
// S2: the files' final hosts (behind the add-ons' redirects): Nuvio's
// connections there (the converter's included) and the TV player's (none: it
// only talks to 127.0.0.1).
static void vigiarHosts(Sessao *s) {
  char h[TBA_FONTES][128], u[TBA_FONTES][160];
  int k;
  for (k = 0; k < TBA_FONTES; k++) {
    h[k][0] = u[k][0] = 0;
    if (k < s->nfo && s->fo[k].eng) dts_engine_final_host(s->fo[k].eng, h[k], sizeof h[k]);
    if (h[k][0]) snprintf(u[k], sizeof u[k], "https://%.140s/", h[k]);
  }
  skVigiar(u[0], u[1], NULL, NULL);
}

// The master's STREAM-INF: what the converter's own master says (size, frame
// rate, HDR range), from the file's video stream.
static void videoInfo(Sessao *s) {
  AVFormatContext *ic = s->fo[0].ic;
  int i;
  int64_t tam = dts_engine_size(s->fo[0].eng);
  for (i = 0; ic && i < (int)ic->nb_streams; i++) {
    AVStream *st = ic->streams[i];
    AVCodecParameters *p = st->codecpar;
    AVRational r;
    if (p->codec_type != AVMEDIA_TYPE_VIDEO || (st->disposition & AV_DISPOSITION_ATTACHED_PIC)) continue;
    s->largura = p->width; s->altura = p->height;
    r = st->avg_frame_rate.num > 0 ? st->avg_frame_rate : st->r_frame_rate;
    s->fps1000 = r.num > 0 && r.den > 0 ? lround(av_q2d(r) * 1000.0) : 0;
    snprintf(s->faixaVideo, sizeof s->faixaVideo, "%s", p->color_trc == AVCOL_TRC_SMPTE2084 ? "PQ" :
             p->color_trc == AVCOL_TRC_ARIB_STD_B67 ? "HLG" :
             av_packet_side_data_get(p->coded_side_data, p->nb_coded_side_data, AV_PKT_DATA_MASTERING_DISPLAY_METADATA) ? "PQ" : "");
    break;
  }
  s->banda = tam > 0 && s->fo[0].dur > 1 ? (long)(tam * 8.0 / s->fo[0].dur) : 20000000L;
  if (s->banda < 1000000L) s->banda = 1000000L;
}

// Watches the donor's opening: past DOADOR_PRAZO_MS its reader is cancelled
// (prepFonte then fails) and the session goes on with the file's tracks.
static void *vigiaDoador(void *u) {
  Sessao *s = u;
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += DOADOR_PRAZO_MS / 1000;
  pthread_mutex_lock(&s->m);
  while (!s->doadorFeito && !s->cancel)
    if (pthread_cond_timedwait(&s->c, &s->m, &ts) == ETIMEDOUT) break;
  if (!s->doadorFeito && !s->cancel && s->fo[1].eng) {
    s->doadorVencido = 1;
    dts_engine_cancel(s->fo[1].eng);
  }
  pthread_mutex_unlock(&s->m);
  soltar(s);
  return NULL;
}

// The donor against the file (as the webOS engine, dts_engine.c): a debrid
// file not ready yet answers with a short clip that plays normally. Under two
// minutes for a picture of ten or more, or no audio and far shorter: the
// placeholder clip (the app retries it). Under half the file: not this title.
// 24/23.976, 25/24, 25/23.976 and back (webOS known_speed).
static int velocidadeConhecida(double k) {
  static const double conh[] = { 24000.0 / 23976.0, 25.0 / 24.0, 25000.0 / 23976.0 };
  int i;
  for (i = 0; i < 3; i++)
    if (fabs(k - conh[i]) < 0.15 * (conh[i] - 1.0) || fabs(k - 1.0 / conh[i]) < 0.15 * (conh[i] - 1.0)) return 1;
  return 0;
}
static double fpsDe(AVFormatContext *ic) {
  int i;
  for (i = 0; ic && i < (int)ic->nb_streams; i++) {
    AVStream *st = ic->streams[i];
    AVRational r;
    if (st->codecpar->codec_type != AVMEDIA_TYPE_VIDEO || (st->disposition & AV_DISPOSITION_ATTACHED_PIC)) continue;
    r = st->avg_frame_rate.num > 0 ? st->avg_frame_rate : st->r_frame_rate;
    return r.num > 0 && r.den > 0 ? av_q2d(r) : 0;
  }
  return 0;
}
// Shorter than this (s) in the menu's track list: the placeholder clip. The
// host test's fixtures are short, it lowers it.
#ifndef DOA_CLIP_S
#define DOA_CLIP_S 120
#endif
static int conferirDoador(Sessao *s) {
  Fonte *a = &s->fo[0], *b = &s->fo[1];
  int i, audio = 0;
  double fa = fpsDe(a->ic), fb = fpsDe(b->ic);
  for (i = 0; i < s->nfx; i++) audio += s->fx[i].fonte == 1;
  if ((!audio && b->dur > 0 && b->dur < a->dur / 2) || (b->dur > 0 && b->dur < 120 && a->dur > 600)) {
    snprintf(b->motivo, sizeof b->motivo, "%s", "External audio: this version is not ready on the debrid service yet (placeholder clip)");
    b->falhou = 1; return 0;
  }
  if (!audio) { snprintf(b->motivo, sizeof b->motivo, "%s", "External audio: the other version has no audio track"); b->falhou = 1; return 0; }
  if (b->dur > 0 && b->dur < a->dur / 2) {
    snprintf(b->motivo, sizeof b->motivo, "External audio: the other version lasts %.0f s, this one %.0f s (not cached?)", b->dur, a->dur);
    b->falhou = 1; return 0;
  }
  pthread_mutex_lock(&s->m);
  s->fpsRazao = fa > 0 && fb > 0 ? fb / fa : 0;
  s->difVel = fa > 0 && fb > 0 && fabs(fa - fb) / fa > 0.001;
  // No speed asked and a known film/TV ratio the lengths confirm (a French
  // 25 fps version of a 23.976 picture): that speed from the start, so a
  // match has only the offset left to find (webOS, owner's log 14/11).
  s->difPpm = 0;
  if (!s->ppmPedido && s->difVel && velocidadeConhecida(s->fpsRazao) && a->dur > 0 &&
      fabs(b->dur * s->fpsRazao / a->dur - 1.0) < 0.01) {
    s->fo[1].k = s->fpsRazao;
    s->difPpm = (int)lround((s->fpsRazao - 1.0) * 1e6);
    printf("[doador] donor at another speed (frame rates %.3f / %.3f, the lengths agree): played at ratio %.6f\n", fb, fa, s->fpsRazao);
  }
  // In the file's time: a sped-up version is not another cut.
  s->difCorte = (int)lround(b->dur * (s->difVel ? s->fpsRazao : 1.0) - a->dur);
  s->difOk = 1;
  pthread_mutex_unlock(&s->m);
  // Played at another speed: its tracks converted (a copy keeps its own pace).
  if (b->k != 1.0)
    for (i = 0; i < s->nfx; i++) {
      Faixa *f = &s->fx[i];
      if (f->fonte != 1 || f->modo == M_AC3 || f->modo == M_AAC || f->modo == M_FORA) continue;
      f->modo = !avcodec_find_decoder(f->id) ? M_FORA : f->canais > 2 ? M_AC3 : M_AAC;
      printf("[doador]   donor audio %d at ratio %.6f -> %s\n", f->ord, b->k, MODO_NOME[f->modo]);
    }
  printf("[doador] file fps %.3f %.1f s, donor fps %.3f %.1f s%s\n", fa, a->dur, fb, b->dur,
         s->difVel ? ": frame rates differ, its audio would drift" : "");
  return 1;
}

static void *preparar(void *u) {
  Sessao *s = u;
  double t0 = agoraMs();
  int ord = s->audio, i;
  if (!s->cancel && (prepFonte(s, 0) || (falhar(s, s->fo[0].motivo[0] ? s->fo[0].motivo : "original file did not open"), 0)) &&
      !s->cancel) {
    if (s->nfo > 1) {
      pthread_t tv;
      int ok;
      atomic_fetch_add(&s->refs, 1);
      if (pthread_create(&tv, NULL, vigiaDoador, s) == 0) pthread_detach(tv); else atomic_fetch_sub(&s->refs, 1);
      ok = prepFonte(s, 1);
      pthread_mutex_lock(&s->m);
      s->doadorFeito = 1;
      if (s->doadorVencido) { ok = 0; s->fo[1].falhou = 1; snprintf(s->fo[1].motivo, sizeof s->fo[1].motivo, "donor: no answer in %d s", DOADOR_PRAZO_MS / 1000); }
      pthread_cond_broadcast(&s->c);
      pthread_mutex_unlock(&s->m);
      if (ok) ok = conferirDoador(s);
      if (ok) {
        s->fo[1].desloc = s->fo[0].v0 - s->fo[1].v0 * s->fo[1].k + s->fo[1].desloc;   // desloc held the offset (ms / 1000) until here
        printf("[doador] donor \"%s\": %.1f s against the file's %.1f s (%+.1f s), starts %.3f vs %.3f s; its time %+.3f s = the file's\n",
               s->fo[1].nome, s->fo[1].dur, s->fo[0].dur, s->fo[1].dur - s->fo[0].dur, s->fo[1].v0, s->fo[0].v0, s->fo[1].desloc);
      } else {
        printf("[doador] donor not offered: %s (the file's own tracks go on)\n", s->fo[1].motivo);
        if (s->fo[1].eng) { dts_engine_cancel(s->fo[1].eng); dts_engine_destroy(s->fo[1].eng); s->fo[1].eng = NULL; }
        s->fo[1].ic = NULL; s->fo[1].conectada = 0;
      }
      fflush(stdout);
    }
    escolherInicial(s, ord);
    if (s->audio < 0) { falhar(s, "no audio track Nuvio can serve"); soltar(s); return NULL; }
    s->pk = av_packet_alloc(); s->fr = av_frame_alloc(); s->ef = av_frame_alloc();
    if (!s->pk || !s->fr || !s->ef) { falhar(s, "out of memory"); soltar(s); return NULL; }
    videoInfo(s);
    // The converter's playlist runs from 0 to the container's end (dts_remux).
    s->dur = s->fo[0].fim;
    s->naSeg = (int)floor(s->dur / AUD_SEG_S);   // a tail under 0.5 s joins the last segment
    if (s->dur - s->naSeg * AUD_SEG_S > 0.5) s->naSeg++;
    if (s->naSeg < 1) s->naSeg = 1;
    s->aIni = malloc(sizeof *s->aIni * (size_t)(s->naSeg + 1));
    if (s->aIni) { int k; for (k = 0; k < s->naSeg; k++) s->aIni[k] = k * AUD_SEG_S; s->aIni[s->naSeg] = s->dur; }
    // Both sources read their header and index: the one not starting gives
    // its connection back now (one audio connection).
    if (s->umaConexao)
      for (i = 0; i < s->nfo; i++) if (i != s->fx[s->audio].fonte) fonteSoltar(s, i, "not the starting track's source");
    vigiarHosts(s);
    pthread_mutex_lock(&s->m);
    if (!s->falhou && s->aIni) {
      s->pronto = 1;
      printf("[doador] ready in %.0f ms: %.1f s, %d audio segments; audio %s %d%s\n", agoraMs() - t0, s->dur, s->naSeg,
             s->fx[s->audio].fonte ? "donor" : "file", s->fx[s->audio].ord, s->nfo > 1 && !s->fo[1].falhou ? ", donor offered" : "");
    } else if (!s->falhou) { s->falhou = 1; snprintf(s->motivo, sizeof s->motivo, "out of memory"); }
    pthread_cond_broadcast(&s->c);
    pthread_mutex_unlock(&s->m);
    fflush(stdout);
  } else if (!s->falhou) falhar(s, s->cancel ? "closed" : "preparation failed");
  soltar(s);
  return NULL;
}

// ---- generator: packets of the active track, in source time --------------------
// ---- automatic match and following another cut (as the webOS engine) -------------
#define MATCH_ESPERA_S   120     // the two captures fill while playing; paused longer: given up
#define MATCH_MAX_MS     3600000 // beyond the sync's reach: never applied
#define SEGUIR_A_CADA_S  120.0
typedef struct { Sessao *s; int automatico, seguir; } MatchPedido;

// The file's audio the donor is compared with: the track in the donor
// track's language (a dub keeps the music and effects), else its first one
// with a decoder.
static int matchSiArquivo(Sessao *s, const Faixa *d) {
  int i, primeiro = -1;
  for (i = 0; i < s->nfx; i++) {
    const Faixa *f = &s->fx[i];
    if (f->fonte != 0 || !avcodec_find_decoder(f->id)) continue;
    if (primeiro < 0) primeiro = f->si;
    if (d && casa(f->lingua, d->lingua)) return f->si;
  }
  return primeiro;
}
static void tapArquivo(void *op, const AVPacket *p, const AVStream *st) {
  Sessao *s = op;
  pthread_mutex_lock(&s->mm);
  ladoPacote(&s->mArq, p, st);
  pthread_mutex_unlock(&s->mm);
}
static void matchIniciar(Sessao *s, int automatico, int seguir);
// The donor's packets the generator reads (its playing track): the match's
// donor side, and the follow's next look when it is due.
static void doadorOuvir(Sessao *s, const AVPacket *p, const AVStream *st) {
  if (s->mArmD) {
    pthread_mutex_lock(&s->mm);
    if (s->mArmD) ladoPacote(&s->mDon, p, st);
    pthread_mutex_unlock(&s->mm);
  } else if (s->segAtivo && !s->mEstado && p->pts != AV_NOPTS_VALUE &&
             p->pts * av_q2d(st->time_base) * s->fo[1].k + s->fo[1].desloc >= s->segProx)
    matchIniciar(s, 1, 1);
}
static double baseDe(const Sessao *s, double k) { return s->fo[0].v0 - s->fo[1].v0 * k; }
static int msDe(const Sessao *s, double off, double k) { return (int)lround((off - baseDe(s, k)) * 1000.0); }
static void deslocAplicar(Sessao *s, int ms) {
  pthread_mutex_lock(&s->gm);
  s->fo[1].desloc = baseDe(s, s->fo[1].k) + ms / 1000.0;
  s->doadorMs = ms;
  if (s->gFonte == 1) s->gOk = 0;
  pthread_mutex_unlock(&s->gm);
}
// The best places over several searches, 0.5 s apart, best first.
static void candAdd(int *n, double *off, double *sc, double o, double v) {
  int i, k;
  for (i = 0; i < *n; i++) if (fabs(off[i] - o) < 0.5) { if (v > sc[i]) { sc[i] = v; off[i] = o; } goto ordem; }
  if (*n < DTS_MATCH_CAND) { off[*n] = o; sc[*n] = v; (*n)++; }
  else if (v > sc[*n - 1]) { off[*n - 1] = o; sc[*n - 1] = v; }
ordem:
  for (i = 1; i < *n; i++) for (k = i; k > 0 && sc[k] > sc[k - 1]; k--) {
    double t = sc[k]; sc[k] = sc[k - 1]; sc[k - 1] = t; t = off[k]; off[k] = off[k - 1]; off[k - 1] = t;
  }
}
static int tentarConfirmar(const DtsMatchSig *sm, const DtsMatchSig *sd, double k, DtsMatchResult *r) {
  int i;
  for (i = 0; i < r->ncand; i++)
    if (r->cand_score[i] >= 0.10 && dts_match_verify(sm, sd, k, r->cand_off[i])) {
      printf("[doador] match: %.3f s (score %.2f) confirmed on both halves of the capture\n", r->cand_off[i], r->cand_score[i]);
      r->offset_s = r->cand_off[i]; r->score = r->cand_score[i]; r->confident = 1;
      return 1;
    }
  return 0;
}
static void respiro(void) { usleep(1000); }
// A wide search's capture: the donor read on its own from `de` for `seg` s.
static int capturar(Sessao *s, int si, double de, double seg, Lado *out) {
  DtsEngine *e = dts_engine_create();
  AVFormatContext *ic;
  AVPacket *pk;
  int i, v = -1, r = -1;
  if (!e) return -1;
  pthread_mutex_lock(&s->m);
  if (s->cancel) { pthread_mutex_unlock(&s->m); dts_engine_destroy(e); return -1; }
  s->mEng = e;
  pthread_mutex_unlock(&s->m);
  if (dts_engine_open_demux(e, s->fo[1].url, s->fo[1].cab) == 0 && (ic = dts_engine_avformat(e)) && si < (int)ic->nb_streams &&
      ladoPreparar(out, si, (int)(seg * DTS_MATCH_HZ) + DTS_MATCH_HZ) == 0) {
    for (i = 0; i < (int)ic->nb_streams; i++) {
      if (v < 0 && ic->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) v = i;
      if (i != si) ic->streams[i]->discard = AVDISCARD_ALL;
    }
    if (v < 0) v = si;
    av_seek_frame(ic, v, av_rescale_q((int64_t)(de * AV_TIME_BASE), AV_TIME_BASE_Q, ic->streams[v]->time_base), AVSEEK_FLAG_BACKWARD);
    if ((pk = av_packet_alloc())) {
      double fim = de + seg + 60.0;
      while (!s->cancel && av_read_frame(ic, pk) >= 0) {
        int cheio = 0;
        if (pk->stream_index == si) {
          cheio = ladoPacote(out, pk, ic->streams[si]);
          if (pk->pts != AV_NOPTS_VALUE && pk->pts * av_q2d(ic->streams[si]->time_base) > fim) cheio = 1;
        }
        av_packet_unref(pk);
        if (cheio) break;
      }
      av_packet_free(&pk);
    }
    if (out->ev.n >= 5 * DTS_MATCH_HZ && out->temT0) r = 0;
  }
  pthread_mutex_lock(&s->m); s->mEng = NULL; pthread_mutex_unlock(&s->m);
  dts_engine_destroy(e);
  return r;
}
static void matchFim(Sessao *s, int achou, const DoaDopasuj *r) {
  pthread_mutex_lock(&s->m);
  s->mRes = *r; s->mEstado = achou ? 2 : 3; s->mNovo = 1;
  pthread_mutex_unlock(&s->m);
}
static void *matchFio(void *u) {
  MatchPedido *pd = u;
  Sessao *s = pd->s;
  int seguir = pd->seguir, i, q, nk, achou = 0, como = 0, ncand = 0;
  double t0 = agoraMs(), k0, kf, ks[2], center, off = 0, razao = 1, score = 0, cand[DTS_MATCH_CAND], candSc[DTS_MATCH_CAND];
  DtsMatchSig sm, sd;
  DoaDopasuj res;
  free(pd);
#ifdef SYS_gettid
  setpriority(PRIO_PROCESS, (id_t)syscall(SYS_gettid), 10);
#endif
  dts_match_respiro = respiro;
  memset(&res, 0, sizeof res);
  // The two captures fill while playing (~40 s).
  for (;;) {
    int prontos, tocaDoador;
    pthread_mutex_lock(&s->mm);
    prontos = ladoCheio(&s->mArq) && ladoCheio(&s->mDon);
    pthread_mutex_unlock(&s->mm);
    tocaDoador = s->audio >= 0 && s->fx[s->audio].fonte == 1;
    if (prontos || s->cancel || !tocaDoador || agoraMs() - t0 > MATCH_ESPERA_S * 1000.0) break;
    usleep(200000);
  }
  dts_remux_set_tap(NULL, s, -1);
  pthread_mutex_lock(&s->mm); s->mArmD = 0; pthread_mutex_unlock(&s->mm);
  ladoSig(&s->mArq, &sm); ladoSig(&s->mDon, &sd);
  printf("[doador] match%s: captured %.1f s of the file at %.1f s and %.1f s of the donor at %.1f s in %.0f ms\n", seguir ? " (follow)" : "",
         sm.n / (double)DTS_MATCH_HZ, sm.t0, sd.n / (double)DTS_MATCH_HZ, sd.t0, agoraMs() - t0);
  k0 = s->fo[1].k; kf = s->fpsRazao;
  ks[0] = k0; ks[1] = kf; nk = kf > 0 && fabs(kf - k0) > 0.0005 && !seguir ? 2 : 1;
  center = s->fo[1].desloc;
  if (!s->cancel && sm.n >= (DTS_MATCH_WIN_S + 10) * DTS_MATCH_HZ && sd.n >= (DTS_MATCH_WIN_S + 10) * DTS_MATCH_HZ) {
    // 1. Near the current offset, from what was played.
    for (q = 0; q < nk; q++) {
      DtsMatchResult r;
      int got = dts_match_search(&sm, &sd, ks[q], center, 10.0, &r);
      if (got && !r.confident) tentarConfirmar(&sm, &sd, ks[q], &r);
      if (got && q == 0) for (i = 0; i < r.ncand; i++) candAdd(&ncand, cand, candSc, r.cand_off[i], r.cand_score[i]);
      printf("[doador] match: ratio %.5f: %s offset %d ms (was %d) score %.2f margin %.2f\n", ks[q],
             got && r.confident ? "found" : "not trusted", got ? msDe(s, r.offset_s, ks[q]) : 0, s->doadorMs, got ? r.score : 0.0, got ? r.margin : 0.0);
      if (got && r.confident && (!achou || r.score > score)) { achou = 1; como = 1; off = r.offset_s; razao = ks[q]; score = r.score; }
    }
    // 2. Not near (a click, not the follow): the donor read on its own where
    // the lengths suggest, +-60 s, +-5 min (webOS match_job_run).
    if (!achou && !seguir && !s->cancel) {
      const double tm = sm.t0 + sm.n / 2.0 / DTS_MATCH_HZ, base = baseDe(s, k0), pad = DTS_MATCH_WIN_S / 2.0 + 5.0;
      int64_t total = s->fo[1].eng ? dts_engine_size(s->fo[1].eng) : 0;
      const double bps = total > 0 && s->fo[1].dur > 0 ? total / s->fo[1].dur : 0;
      int corte = s->difCorte, longe = abs(corte) > 600, n = 0;
      struct { double c, r, capMb; int como; } st[5];
      if (abs(corte) > 10) {
        double h1 = -corte + base, h2 = -corte * (s->fo[0].dur > 0 ? tm / s->fo[0].dur : 0) + base;
        if (!longe) { st[n].c = h1; st[n].r = 10; st[n].capMb = 1e9; st[n++].como = 2; }
        if (longe || (fabs(h2 - h1) > 15 && fabs(h2 - center) > 15)) { st[n].c = h2; st[n].r = longe ? 60 : 10; st[n].capMb = 300; st[n++].como = 2; }
        if (longe) {
          double c = h2, r = 150;
          if (corte < 0 && c - r < base) c = base + r;
          if (corte > 0 && c + r > base) c = base - r;
          st[n].c = c; st[n].r = r; st[n].capMb = 300; st[n++].como = 4;
        }
      }
      st[n].c = center; st[n].r = 60; st[n].capMb = 300; st[n++].como = 3;
      if (!longe) { st[n].c = center; st[n].r = 300; st[n].capMb = 300; st[n++].como = 4; }
      for (i = 0; i < n && !s->cancel && !achou; i++) {
        double lo = 1e18, hi = -1e18, mb;
        Lado lado;
        if (st[i].como == 2 && fabs(st[i].c - center) < 10) continue;   // the near search covered it
        for (q = 0; q < nk; q++) {
          double a = (tm - pad - (st[i].c + st[i].r)) / ks[q], b = (tm + pad - (st[i].c - st[i].r)) / ks[q];
          if (a < lo) lo = a;
          if (b > hi) hi = b;
        }
        lo -= 2.0; hi += 2.0;
        if (lo < 0) lo = 0;
        if (hi <= lo + 5.0) continue;
        mb = bps * (hi - lo) / 1048576.0;
        if (bps > 0 && mb > st[i].capMb) {
          printf("[doador] match: +-%.0f s skipped, about %.0f MB of the donor to read\n", st[i].r, mb);
          res.pesado = 1; continue;
        }
        printf("[doador] match: %s, reading %.0f s of the donor (about %.0f MB)\n",
               st[i].como == 2 ? "trying the offset its length suggests" : st[i].r < 100 ? "searching +-60 s" : "searching +-5 min", hi - lo, mb);
        fflush(stdout);
        memset(&lado, 0, sizeof lado);
        if (capturar(s, s->mDon.si, lo, hi - lo, &lado) == 0) {
          DtsMatchSig sw;
          ladoSig(&lado, &sw);
          for (q = 0; q < nk; q++) {
            DtsMatchResult r;
            int got = dts_match_search(&sm, &sw, ks[q], st[i].c, st[i].r, &r);
            if (got && !r.confident) tentarConfirmar(&sm, &sw, ks[q], &r);
            if (got && r.confident && (!achou || r.score > score)) { achou = 1; como = st[i].como; off = r.offset_s; razao = ks[q]; score = r.score; }
            if (got && q == 0) for (int c = 0; c < r.ncand; c++) candAdd(&ncand, cand, candSc, r.cand_off[c], r.cand_score[c]);
            printf("[doador] match: ratio %.5f around %.1f s: offset %.3f s score %.2f margin %.2f%s\n", ks[q], st[i].c - base,
                   r.offset_s - base, r.score, r.margin, r.confident ? " (trusted)" : "");
          }
        }
        ladoLiberar(&lado);
      }
    }
  }
  for (i = 0; i < ncand && res.ncand < 3; i++) {
    int m = msDe(s, cand[i], k0);
    if (m > MATCH_MAX_MS || m < -MATCH_MAX_MS) continue;
    res.cand_ms[res.ncand] = m; res.cand_pct[res.ncand++] = (int)lround(candSc[i] * 100.0);
  }
  if (achou) {
    res.ms = msDe(s, off, razao); res.como = como;
    res.ppm = razao != 1.0 ? (int)lround((razao - 1.0) * 1e6) : 0;
    if (res.ms > MATCH_MAX_MS || res.ms < -MATCH_MAX_MS) {
      printf("[doador] match: %d ms is beyond the sync's reach, not applied\n", res.ms);
      achou = 0;
    }
  }
  if (!achou) res.dica_ms = abs(s->difCorte) > 10 && abs(s->difCorte) <= 600 ? -s->difCorte * 1000 : 0;
  printf("[doador] match%s: %s in %.0f ms%s\n", seguir ? " (follow)" : "", achou ? "found" : "not found", agoraMs() - t0,
         achou ? "" : res.pesado ? " (a wide search skipped for its download)" : "");
  if (seguir) {
    // The follow corrects the offset by itself; the app hears of it.
    pthread_mutex_lock(&s->m);
    if (achou && fabs(razao - k0) < 1e-6) {
      s->segFalhas = 0; s->segPerdido = 0;
      if (abs(res.ms - s->doadorMs) > 250) {
        pthread_mutex_unlock(&s->m);
        deslocAplicar(s, res.ms);
        pthread_mutex_lock(&s->m);
        s->segEvento = 4; s->segMs = res.ms;
        printf("[doador] follow: a shift of %+d ms (a gap longer in one version): offset %+d ms now\n", res.ms - s->doadorMs, res.ms);
      }
    } else if (++s->segFalhas >= 2 && !s->segPerdido) {
      s->segPerdido = 1; s->segEvento = 3; s->segMs = s->doadorMs;
      printf("[doador] follow: lost it (two looks without a trusted place): use Match automatically\n");
    }
    s->segProx = sm.t0 + sm.n / (double)DTS_MATCH_HZ + SEGUIR_A_CADA_S;
    s->mEstado = 0;
    pthread_mutex_unlock(&s->m);
  } else {
    matchFim(s, achou, &res);
    // Another cut and "Follow through the whole film": from this find on.
    if (achou && s->seguir && abs(s->difCorte) > 10) {
      pthread_mutex_lock(&s->m);
      s->segAtivo = 1; s->segFalhas = 0; s->segPerdido = 0;
      s->segProx = sm.t0 + sm.n / (double)DTS_MATCH_HZ + SEGUIR_A_CADA_S;
      pthread_mutex_unlock(&s->m);
      printf("[doador] follow: on (the lengths differ by %d s), next look in %.0f s\n", s->difCorte, SEGUIR_A_CADA_S);
    }
  }
  fflush(stdout);
  soltar(s);
  return NULL;
}
static void matchIniciar(Sessao *s, int automatico, int seguir) {
  MatchPedido *pd;
  pthread_t t;
  const Faixa *d;
  int siA;
  pthread_mutex_lock(&s->m);
  if (s->mEstado == 1 || s->cancel || !s->pronto || s->audio < 0 || s->fx[s->audio].fonte != 1) {
    int ocupado = s->mEstado == 1;
    pthread_mutex_unlock(&s->m);
    if (!ocupado && !seguir) { DoaDopasuj z; memset(&z, 0, sizeof z); matchFim(s, 0, &z); }
    return;
  }
  s->mEstado = 1; s->mAuto = automatico;
  pthread_mutex_unlock(&s->m);
  d = &s->fx[s->audio];
  siA = matchSiArquivo(s, d);
  pd = malloc(sizeof *pd);
  pthread_mutex_lock(&s->mm);
  if (siA < 0 || !pd || ladoPreparar(&s->mArq, siA, DTS_MATCH_MAX) < 0 || ladoPreparar(&s->mDon, d->si, DTS_MATCH_MAX) < 0) {
    pthread_mutex_unlock(&s->mm);
    free(pd);
    pthread_mutex_lock(&s->m); s->mEstado = 0; pthread_mutex_unlock(&s->m);
    if (!seguir) { DoaDopasuj z; memset(&z, 0, sizeof z); matchFim(s, 0, &z); }
    return;
  }
  s->mArmD = 1;
  pthread_mutex_unlock(&s->mm);
  dts_remux_set_tap(tapArquivo, s, siA);
  pd->s = s; pd->automatico = automatico; pd->seguir = seguir;
  atomic_fetch_add(&s->refs, 1);
  if (pthread_create(&t, NULL, matchFio, pd) != 0) {
    atomic_fetch_sub(&s->refs, 1); free(pd);
    dts_remux_set_tap(NULL, s, -1);
    pthread_mutex_lock(&s->mm); s->mArmD = 0; pthread_mutex_unlock(&s->mm);
    pthread_mutex_lock(&s->m); s->mEstado = 0; pthread_mutex_unlock(&s->m);
    return;
  }
  pthread_detach(t);
  printf("[doador] match%s%s: capturing ~%d s of the file's audio %d and the donor's %d while playing\n", seguir ? " (follow)" : "",
         automatico ? " (by itself)" : "", DTS_MATCH_SECONDS, siA, d->si);
  fflush(stdout);
}

static int filaPor(Sessao *s, AVPacket *p, double t) {
  int k;
  if (s->fN == FILA_MAX) { av_packet_free(&p); return 0; }
  k = (s->fIni + s->fN) % FILA_MAX;
  s->fila[k].p = p; s->fila[k].t = t; s->fN++;
  return 1;
}
static int abrirConversor(Sessao *s, const Faixa *f, char *err, size_t en) {
  AVStream *st = s->fo[f->fonte].ic->streams[f->si];
  const AVCodec *dc = avcodec_find_decoder(f->id);
  const AVCodec *ec = avcodec_find_encoder(f->modo == M_AC3 ? AV_CODEC_ID_AC3 : AV_CODEC_ID_AAC);
  AVChannelLayout alvo;
  int r, ch = f->modo == M_AC3 && f->canais > 2 ? 6 : 2;
  codecLiberar(s);
  if (!dc || !ec) { snprintf(err, en, "decoder/encoder not in this build (%s)", f->codec); return -1; }
  s->dec = avcodec_alloc_context3(dc);
  if (!s->dec) { snprintf(err, en, "out of memory"); return -1; }
  avcodec_parameters_to_context(s->dec, st->codecpar);
  s->dec->pkt_timebase = st->time_base;
  av_channel_layout_default(&alvo, ch);
  if (f->id == AV_CODEC_ID_DTS) av_opt_set_int(s->dec->priv_data, "core_only", 1, 0);
  else if ((f->id == AV_CODEC_ID_TRUEHD || f->id == AV_CODEC_ID_MLP) && (f->canais == 0 || f->canais > ch))
    av_opt_set_chlayout(s->dec->priv_data, "downmix", &alvo, 0);
  if ((r = avcodec_open2(s->dec, dc, NULL)) < 0) { snprintf(err, en, "audio decoder open (%d)", r); return -1; }
  s->enc = avcodec_alloc_context3(ec);
  if (!s->enc) { snprintf(err, en, "out of memory"); return -1; }
  av_channel_layout_copy(&s->enc->ch_layout, &alvo);
  s->enc->sample_rate = 48000; s->enc->time_base = (AVRational){1, 48000};
  s->enc->sample_fmt = AV_SAMPLE_FMT_FLTP;
  s->enc->bit_rate = f->modo == M_AC3 ? (ch == 6 ? 640000 : 192000) : 192000;
  if (f->modo == M_AAC) { s->enc->profile = AV_PROFILE_AAC_LOW; s->enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; }
  if ((r = avcodec_open2(s->enc, ec, NULL)) < 0) { snprintf(err, en, "audio encoder open (%d)", r); return -1; }
  s->fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLTP, ch, 8192);
  if (!s->fifo) { snprintf(err, en, "out of memory"); return -1; }
  s->base48 = AV_NOPTS_VALUE; s->amostras = 0;
  av_channel_layout_uninit(&alvo);
  return 0;
}
static int encodar(Sessao *s, AVFrame *quadro) {
  int r = avcodec_send_frame(s->enc, quadro);
  if (r < 0 && r != AVERROR_EOF) return -1;
  for (;;) {
    AVPacket *o = av_packet_alloc();
    if (!o) return -1;
    r = avcodec_receive_packet(s->enc, o);
    if (r < 0) { av_packet_free(&o); return r == AVERROR(EAGAIN) || r == AVERROR_EOF ? 0 : -1; }
    filaPor(s, o, o->pts / 48000.0 + s->gDesloc);   // the playing file's time
  }
}
static int drenarFifo(Sessao *s, int tudo) {
  int fs = s->enc->frame_size > 0 ? s->enc->frame_size : 1536;
  while (av_audio_fifo_size(s->fifo) >= fs || (tudo && av_audio_fifo_size(s->fifo) > 0)) {
    int n = av_audio_fifo_size(s->fifo) < fs ? av_audio_fifo_size(s->fifo) : fs;
    av_frame_unref(s->ef);
    s->ef->nb_samples = fs; s->ef->format = AV_SAMPLE_FMT_FLTP; s->ef->sample_rate = 48000;
    av_channel_layout_copy(&s->ef->ch_layout, &s->enc->ch_layout);
    if (av_frame_get_buffer(s->ef, 0) < 0) return -1;
    if (n < fs) av_samples_set_silence(s->ef->data, 0, fs, s->ef->ch_layout.nb_channels, AV_SAMPLE_FMT_FLTP);
    av_audio_fifo_read(s->fifo, (void **)s->ef->data, n);
    s->ef->pts = s->base48 + s->amostras;
    s->amostras += fs;
    if (encodar(s, s->ef) < 0) return -1;
  }
  return 0;
}
// One more step: >0 progressed, 0 end of file, -1 error.
static int produzir(Sessao *s, const Faixa *f, char *err, size_t en) {
  Fonte *fo = &s->fo[f->fonte];
  AVStream *st = fo->ic->streams[f->si];
  for (;;) {
    int r;
    if (s->cancel) { snprintf(err, en, "closed"); return -1; }
    if (s->eof) return 0;
    r = av_read_frame(fo->ic, s->pk);
    fo->leu = 1;
    if (r < 0) {
      if (r != AVERROR_EOF) { snprintf(err, en, "reading the %s (%d)", f->fonte ? "donor" : "original file", r); return -1; }
      s->eof = 1;
      if (s->enc) { drenarFifo(s, 1); encodar(s, NULL); }
      return 0;
    }
    if (s->pk->stream_index != f->si) { av_packet_unref(s->pk); continue; }
    if (s->pk->pts == AV_NOPTS_VALUE) s->pk->pts = s->pk->dts;
    if (s->pk->pts == AV_NOPTS_VALUE) { av_packet_unref(s->pk); continue; }
    if (f->fonte == 1) doadorOuvir(s, s->pk, st);
    if (!s->dec) {
      AVPacket *c = av_packet_clone(s->pk);
      double t = s->pk->pts * av_q2d(st->time_base) + s->gDesloc;   // the playing file's time
      av_packet_unref(s->pk);
      if (!c) { snprintf(err, en, "out of memory"); return -1; }
      filaPor(s, c, t);
      return 1;
    }
    r = avcodec_send_packet(s->dec, s->pk);
    av_packet_unref(s->pk);
    if (r < 0 && r != AVERROR(EAGAIN)) continue;   // a broken frame: skip it
    for (;;) {
      r = avcodec_receive_frame(s->dec, s->fr);
      if (r < 0) break;
      if (!s->swr) {
        // Another speed: the samples are made at 48 kHz * k and taken as 48
        // kHz (the audio lasts k times longer, its pitch follows).
        if (swr_alloc_set_opts2(&s->swr, &s->enc->ch_layout, AV_SAMPLE_FMT_FLTP, (int)lround(48000.0 * fo->k), &s->fr->ch_layout,
                                s->fr->format, s->fr->sample_rate, 0, NULL) < 0 || swr_init(s->swr) < 0) {
          snprintf(err, en, "resampler"); av_frame_unref(s->fr); return -1;
        }
      }
      if (s->base48 == AV_NOPTS_VALUE) {
        int64_t p = s->fr->pts != AV_NOPTS_VALUE ? s->fr->pts : s->fr->best_effort_timestamp;
        s->base48 = p != AV_NOPTS_VALUE ? llround(p * av_q2d(st->time_base) * fo->k * 48000.0) : 0;
      }
      { int max = swr_get_out_samples(s->swr, s->fr->nb_samples);
        uint8_t **buf = NULL;
        if (max > 0 && av_samples_alloc_array_and_samples(&buf, NULL, s->enc->ch_layout.nb_channels, max,
                                                          AV_SAMPLE_FMT_FLTP, 0) >= 0) {
          int k = swr_convert(s->swr, buf, max, (const uint8_t **)s->fr->extended_data, s->fr->nb_samples);
          if (k > 0) av_audio_fifo_write(s->fifo, (void **)buf, k);
          av_freep(&buf[0]); av_freep(&buf);
        } }
      av_frame_unref(s->fr);
      if (drenarFifo(s, 0) < 0) { snprintf(err, en, "audio encoder"); return -1; }
    }
    if (s->fN) return 1;
  }
}
// Stop what is being made: the player hung up (`so` closed) or asked for a
// segment that is not this one or the next (a seek). 1 hung up, 2 superseded.
static int abandonar(Sessao *s, int so, long chave) {
  if (s->cancel) return 1;
  if (so >= 0) {
    struct pollfd p = { so, POLLIN, 0 };
    if (poll(&p, 1, 0) > 0) {
      char c;
      if (p.revents & (POLLHUP | POLLERR)) return 1;
      if (recv(so, &c, 1, MSG_PEEK | MSG_DONTWAIT) == 0) return 1;
    }
  }
  if (atomic_load(&s->espera) > 0) {
    long w = atomic_load(&s->espChave);
    if (w != chave && w != chave + 1) return 2;
  }
  return 0;
}
// Positions the generator for segment `seg` of track `fi` on the list `ini`
// (the 2 s audio segments), whose
// segments are about `len` s. *saltou = 1 when packets before the segment
// must be dropped. A jump of more than one segment seeks: these files run at
// 40 Mbit/s, reading 15 s through took 6.4 s (log 14/11).
static int posicionar(Sessao *s, int fi, int seg, const double *ini, double len, int *saltou, const char **como,
                      char *err, size_t en) {
  const Faixa *f = &s->fx[fi];
  Fonte *fo = &s->fo[f->fonte];
  // Segment `seg` in the playing file's time, then in this source's own.
  double alvo = (ini[seg] + tl0(s) - fo->desloc) / fo->k;
  int mesmo = s->gOk && s->gFaixa == fi && s->gModo == f->modo && s->gIni == ini && s->gSeg >= 0 && s->gDesloc == fo->desloc;
  *saltou = 0; *como = "continued";
  if (mesmo && s->gSeg == seg) return 0;
  if (mesmo && seg > s->gSeg && ini[seg] - ini[s->gSeg] <= len + 0.5) { *saltou = 1; *como = "read ahead"; return 0; }
  *saltou = 1; *como = fo->conectada ? "seeked" : "seeked, connection taken again";
  // The other source goes quiet: its connection back (one audio connection).
  if (s->umaConexao && s->gFonte >= 0 && s->gFonte != f->fonte) fonteSoltar(s, s->gFonte, "the player moved to the other source's track");
  if (!fo->conectada) { fo->conectada = 1; fo->nVoltas++; }
  filaLimpar(s);
  s->eof = 0;
  { int i; for (i = 0; i < (int)fo->ic->nb_streams; i++) fo->ic->streams[i]->discard = i == f->si ? AVDISCARD_DEFAULT : AVDISCARD_ALL; }
  if (f->modo == M_AC3 || f->modo == M_AAC) { if (abrirConversor(s, f, err, en) < 0) { s->gOk = 0; return -1; } }
  else codecLiberar(s);
  dts_engine_interrupt(fo->eng, 0);
  s->gFonte = f->fonte; s->gDesloc = fo->desloc;
  if (alvo < fo->v0) alvo = fo->v0;   // a donor that starts later than the file: from its own start
  if (seg > 0 || fo->leu || f->fonte) {
    // Straight to the segment: the index gives the keyframe at or before it.
    AVStream *b = fo->ic->streams[fo->sBusca];
    int64_t ts = av_rescale_q((int64_t)(alvo * AV_TIME_BASE), AV_TIME_BASE_Q, b->time_base);
    // Before the index's first entry Matroska does not move at all (it
    // returns 0 and the reader stays where it was): ask for the first entry.
    if (avformat_index_get_entries_count(b) > 0 && av_index_search_timestamp(b, ts, AVSEEK_FLAG_BACKWARD) < 0) {
      const AVIndexEntry *e0 = avformat_index_get_entry(b, 0);
      if (e0 && e0->timestamp > ts) {
        printf("[doador] seek to %.3f s is before the index (first entry %.3f s): going to the first entry\n", alvo,
               e0->timestamp * av_q2d(b->time_base));
        ts = e0->timestamp;
      }
    }
    if (av_seek_frame(fo->ic, fo->sBusca, ts, AVSEEK_FLAG_BACKWARD) < 0 &&
        av_seek_frame(fo->ic, -1, (int64_t)(alvo * AV_TIME_BASE), AVSEEK_FLAG_BACKWARD) < 0) {
      snprintf(err, en, "seek in the %s to %.1f s", f->fonte ? "donor" : "original file", alvo); s->gOk = 0; return -1;
    }
  }
  s->gOk = 1; s->gFaixa = fi; s->gModo = f->modo; s->gSeg = seg; s->gIni = ini;
  return 0;
}

// ---- muxing into memory ----------------------------------------------------------
typedef struct { AVFormatContext *oc; int64_t cab; } Mux;
static void muxFechar(Mux *m) {
  if (!m->oc) return;
  if (m->oc->pb) { uint8_t *b = NULL; avio_close_dyn_buf(m->oc->pb, &b); av_free(b); m->oc->pb = NULL; }
  avformat_free_context(m->oc);
  m->oc = NULL;
}
// streams: 1 = audio only, 2 = video (from `vpar`) + audio.
static int muxAbrir(Mux *m, const char *fmt, const AVCodecParameters *vpar, AVRational vtb,
                    const AVCodecParameters *apar, AVRational atb, const char *lingua) {
  AVDictionary *o = NULL;
  AVStream *st;
  int r;
  memset(m, 0, sizeof *m);
  if (avformat_alloc_output_context2(&m->oc, NULL, fmt, NULL) < 0 || !m->oc) return -1;
  if (vpar) {
    if (!(st = avformat_new_stream(m->oc, NULL))) return -1;
    avcodec_parameters_copy(st->codecpar, vpar); st->codecpar->codec_tag = 0; st->time_base = vtb;
  }
  if (!(st = avformat_new_stream(m->oc, NULL))) return -1;
  avcodec_parameters_copy(st->codecpar, apar); st->codecpar->codec_tag = 0; st->time_base = atb;
  if (lingua && lingua[0]) av_dict_set(&st->metadata, "language", lingua, 0);
  if (avio_open_dyn_buf(&m->oc->pb) < 0) return -1;
  m->oc->avoid_negative_ts = AVFMT_AVOID_NEG_TS_DISABLED;
  if (!strcmp(fmt, "mp4")) {
    m->oc->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;   // FLAC in MP4
    av_dict_set(&o, "movflags", "frag_custom+empty_moov+default_base_moof+frag_discont", 0);
    av_dict_set(&o, "use_editlist", "0", 0);
  }
  r = avformat_write_header(m->oc, &o);
  av_dict_free(&o);
  if (r < 0) return -1;
  avio_flush(m->oc->pb);
  m->cab = avio_tell(m->oc->pb);
  return 0;
}
// Finishes and hands the bytes (header dropped with `semCabecalho`). malloc'd.
static unsigned char *muxFim(Mux *m, int mp4, int semCabecalho, size_t *n) {
  uint8_t *b = NULL;
  int k, pula;
  unsigned char *o;
  if (mp4) av_write_frame(m->oc, NULL); else av_write_trailer(m->oc);
  k = avio_close_dyn_buf(m->oc->pb, &b);
  m->oc->pb = NULL;
  pula = semCabecalho && m->cab > 0 && m->cab <= k ? (int)m->cab : 0;
  o = malloc((size_t)(k - pula) + 1);
  if (o && k - pula > 0) memcpy(o, b + pula, (size_t)(k - pula));
  *n = o ? (size_t)(k - pula) : 0;
  av_free(b);
  muxFechar(m);
  return o;
}
// Parameters of what goes out for track f (copied, or the encoder's).
static int parametrosSaida(Sessao *s, const Faixa *f, AVCodecParameters *p, AVRational *tb) {
  if (s->enc && (f->modo == M_AC3 || f->modo == M_AAC)) {
    avcodec_parameters_from_context(p, s->enc);
    *tb = (AVRational){1, 48000};
  } else {
    avcodec_parameters_copy(p, s->fo[f->fonte].ic->streams[f->si]->codecpar);
    *tb = s->fo[f->fonte].ic->streams[f->si]->time_base;
  }
  return 0;
}
// Writes one audio packet (its source's time base `tb`) shifted onto the
// video's timeline: its source's time + desloc = the playing file's time, + ptsDesloc.
static int escreverAudio(Sessao *s, Mux *m, int idx, AVPacket *p, AVRational tb, int interl) {
  AVStream *st = m->oc->streams[idx];
  double desloc = ptsDesloc(s) + s->gDesloc;
  int64_t d = llround(desloc / av_q2d(st->time_base));
  if (p->duration <= 0 && s->enc) p->duration = s->enc->frame_size > 0 ? s->enc->frame_size : 1536;
  av_packet_rescale_ts(p, tb, st->time_base);
  if (p->pts != AV_NOPTS_VALUE) p->pts += d;
  if (p->dts != AV_NOPTS_VALUE) p->dts += d; else p->dts = p->pts;
  p->stream_index = idx;
  p->pos = -1;
  return interl ? av_interleaved_write_frame(m->oc, p) : av_write_frame(m->oc, p);
}

typedef struct { unsigned char *b; size_t n; char tipo[48]; int status; } Resp;

// Audio of track fi, segment `seg` (AUD_SEG_S) or -1 = the fMP4 init.
// -2 = stopped (the player hung up or moved on), nothing to send.
static int gerarAudio(Sessao *s, int fi, int seg, Resp *r, char *err, size_t en, int so, long chave) {
  const Faixa *f = &s->fx[fi];
  AVCodecParameters *par = avcodec_parameters_alloc();
  AVRational tb, srcTb;
  Mux m;
  int mp4 = f->modo == M_MP4, saltou = 0, nPk = 0, rr = 0, ab = 0;
  const char *como = "";
  double t0 = agoraMs(), fs, fe, pri = -1, tl;
  if (!par) { snprintf(err, en, "out of memory"); return -1; }
  if (seg < 0) {   // init: the header alone (the converter state is not touched)
    AVStream *as = s->fo[f->fonte].ic->streams[f->si];
    avcodec_parameters_copy(par, as->codecpar);
    if (muxAbrir(&m, "mp4", NULL, (AVRational){0, 1}, par, as->time_base, f->lingua) < 0) {
      muxFechar(&m); avcodec_parameters_free(&par); snprintf(err, en, "fMP4 init (muxer)"); return -1;
    }
    { uint8_t *b = NULL; int k = avio_close_dyn_buf(m.oc->pb, &b); m.oc->pb = NULL;
      r->b = malloc((size_t)k + 1); if (r->b) { memcpy(r->b, b, (size_t)k); r->n = (size_t)k; } av_free(b); }
    muxFechar(&m);
    avcodec_parameters_free(&par);
    snprintf(r->tipo, sizeof r->tipo, "audio/mp4");
    return r->b ? 0 : -1;
  }
  if (posicionar(s, fi, seg, s->aIni, AUD_SEG_S, &saltou, &como, err, en) < 0) { avcodec_parameters_free(&par); return -1; }
  parametrosSaida(s, f, par, &tb);
  srcTb = s->dec ? (AVRational){1, 48000} : s->fo[f->fonte].ic->streams[f->si]->time_base;
  if (muxAbrir(&m, mp4 ? "mp4" : "mpegts", NULL, (AVRational){0, 1}, par, tb, f->lingua) < 0) {
    muxFechar(&m); avcodec_parameters_free(&par); snprintf(err, en, "muxer"); return -1;
  }
  avcodec_parameters_free(&par);
  fs = s->aIni[seg] + tl0(s);   // the playing file's time (a donor's packets are mapped onto it)
  fe = seg + 1 < s->naSeg ? s->aIni[seg + 1] + tl0(s) : 1e18;
  for (;;) {
    Item it;
    if ((ab = abandonar(s, so, chave)) != 0) { rr = -2; break; }
    if (!s->fN) { rr = produzir(s, f, err, en); if (rr < 0) break; if (rr == 0 && !s->fN) { rr = 0; break; } continue; }
    it = s->fila[s->fIni];
    if (it.t >= fe) break;
    s->fIni = (s->fIni + 1) % FILA_MAX; s->fN--;
    if (saltou && it.t < fs - 0.001) { av_packet_free(&it.p); continue; }
    if (pri < 0) pri = it.t;
    if (escreverAudio(s, &m, 0, it.p, srcTb, 0) < 0) { av_packet_free(&it.p); snprintf(err, en, "mux write"); rr = -1; break; }
    av_packet_free(&it.p);
    nPk++;
  }
  tl = agoraMs();
  if (rr == -2) {
    // The packets already taken belong to this segment: the next request
    // seeks (or continues past them only when it is the next segment).
    muxFechar(&m);
    if (ab == 2) s->gOk = 0;
    s->nLarg++;
    snprintf(err, en, "%s after %.0f ms", ab == 1 ? "the player hung up" : "the player asked for another segment", tl - t0);
    return -2;
  }
  if (rr < 0) {
    muxFechar(&m); s->gOk = 0;
    // The donor stopped answering: three of its segments failed in a row.
    if (f->fonte == 1 && ++s->dFalhas >= 3 && !s->dSemResposta) {
      s->dSemResposta = 1;
      printf("[doador] the donor does not respond (3 segments failed in a row)\n");
    }
    return -1;
  }
  if (f->fonte == 1) {
    s->dFalhas = 0;
    // Slower than the picture: a 2 s segment over 10 s, twice.
    if (tl - t0 > 10000.0 && ++s->dLento == 2) printf("[doador] the donor downloads too slowly (two segments over 10 s)\n");
  }
  r->b = muxFim(&m, mp4, mp4, &r->n);
  if (!r->b) { snprintf(err, en, "out of memory"); return -1; }
  snprintf(r->tipo, sizeof r->tipo, mp4 ? "audio/mp4" : "video/mp2t");
  s->gSeg = seg + 1;
  s->nSeg++; s->bytes += (long long)r->n; s->msLer += tl - t0;
  if (!strcmp(como, "seeked, connection taken again")) s->fo[f->fonte].msVolta = tl - t0;
  printf("[doador] %saudio %d seg %d (%.1f-%.1f s): %d packets, %zu KB, %.0f ms, %s; first audio at %.3f s\n",
         f->fonte ? "donor " : "", f->ord, seg, s->aIni[seg], s->aIni[seg + 1], nPk, r->n / 1024, tl - t0, como,
         pri >= 0 ? pri - tl0(s) : -1.0);
  fflush(stdout);
  return 0;
}

// ---- playlists ------------------------------------------------------------------
typedef struct { char *b; size_t n, cap; } Txt;
static void tx(Txt *t, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void tx(Txt *t, const char *fmt, ...) {
  va_list a;
  int k;
  if (!t->b) { t->cap = 65536; t->b = malloc(t->cap); t->n = 0; if (!t->b) return; }
  for (;;) {
    va_start(a, fmt);
    k = vsnprintf(t->b + t->n, t->cap - t->n, fmt, a);
    va_end(a);
    if (k < 0) return;
    if ((size_t)k < t->cap - t->n) { t->n += (size_t)k; return; }
    { char *g = realloc(t->b, t->cap * 2); if (!g) return; t->b = g; t->cap *= 2; }
  }
}
// One audio rendition: AUD_SEG_S segments (short: a seek reads ~2 s).
static void plAudio(Sessao *s, Txt *t, int k) {
  int i, mp4 = s->fx[k].modo == M_MP4;
  tx(t, "#EXTM3U\n#EXT-X-VERSION:%d\n#EXT-X-TARGETDURATION:%d\n#EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-PLAYLIST-TYPE:VOD\n"
        "#EXT-X-INDEPENDENT-SEGMENTS\n", mp4 ? 7 : 3, (int)ceil(AUD_SEG_S));
  if (mp4) tx(t, "#EXT-X-MAP:URI=\"a%d_init.mp4\"\n", k);
  for (i = 0; i < s->naSeg; i++) tx(t, "#EXTINF:%.6f,\na%d_%d%s\n", s->aIni[i + 1] - s->aIni[i], k, i, mp4 ? ".m4s" : ".ts");
  tx(t, "#EXT-X-ENDLIST\n");
}
static void nomeRendicao(const Sessao *s, const Faixa *f, char *o, size_t n) {
  char *c;
  if (f->fonte)   // the donor's: where it comes from, so the TV's own list tells them apart
    snprintf(o, n, "%s (%s %dch) from %.40s", f->nome[0] ? f->nome : nomeLingua(f->lingua), f->codec, f->canais,
             s->fo[1].nome[0] ? s->fo[1].nome : "another source");
  else snprintf(o, n, "%s %d (%s %dch)", f->nome[0] ? f->nome : nomeLingua(f->lingua), f->ord + 1, f->codec, f->canais);
  for (c = o; *c; c++) if (*c == '"' || *c == ',' || (unsigned char)*c < 32) *c = ' ';
}
static void plMaster(Sessao *s, Txt *t) {
  int i, mp4 = 0;
  for (i = 0; i < s->nfx; i++) if (s->fx[i].modo == M_MP4) mp4 = 1;
  tx(t, "#EXTM3U\n#EXT-X-VERSION:%d\n#EXT-X-INDEPENDENT-SEGMENTS\n", mp4 ? 7 : 4);
  for (i = 0; i < s->nfx; i++) {
    const Faixa *f = &s->fx[i];
    char nome[128];
    int ch = f->modo == M_AC3 ? (f->canais > 2 ? 6 : 2) : f->modo == M_AAC ? 2 : f->canais;
    if (f->modo == M_FORA) continue;
    nomeRendicao(s, f, nome, sizeof nome);
    tx(t, "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"aud\",NAME=\"%s\"", nome);
    if (f->lingua[0]) tx(t, ",LANGUAGE=\"%s\"", f->lingua);
    tx(t, ",DEFAULT=%s,AUTOSELECT=YES", i == s->audio ? "YES" : "NO");
    if (ch > 0) tx(t, ",CHANNELS=\"%d\"", ch);
    tx(t, ",URI=\"a%d.m3u8\"\n", i);
  }
  // The video: the converter's media playlist, its segments carry no audio
  // (the TV plays the audio of the video segments when they have one, and
  // ignores the rendition: owner's S95C, 14/11). Integers only (LC_NUMERIC).
  tx(t, "#EXT-X-STREAM-INF:BANDWIDTH=%ld,AUDIO=\"aud\"", s->banda + 1500000L);
  if (s->largura > 0 && s->altura > 0) tx(t, ",RESOLUTION=%dx%d", s->largura, s->altura);
  if (s->fps1000 > 0) tx(t, ",FRAME-RATE=%ld.%03ld", s->fps1000 / 1000, s->fps1000 % 1000);
  if (s->faixaVideo[0]) tx(t, ",VIDEO-RANGE=%s", s->faixaVideo);
  tx(t, "\n%s\n", s->video);
}

// ---- HTTP ---------------------------------------------------------------------------
static int enviar(int so, const void *b, size_t n, Sessao *s) {
  const char *p = b;
  while (n) {
    ssize_t r = send(so, p, n, MSG_NOSIGNAL);
    if (r < 0 && errno == EINTR) continue;
    if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) { if (s && s->cancel) return 0; continue; }
    if (r <= 0) return 0;
    p += r; n -= (size_t)r;
  }
  return 1;
}
static int responder(int so, int cod, const char *tipo, const void *corpo, size_t n, int manter, int head, Sessao *s) {
  char cab[400];
  const char *msg = cod == 200 ? "OK" : cod == 404 ? "Not Found" : cod == 503 ? "Service Unavailable" : "Bad Gateway";
  snprintf(cab, sizeof cab, "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nCache-Control: no-cache\r\n"
           "Connection: %s\r\n\r\n", cod, msg, tipo ? tipo : "text/plain", n, manter ? "keep-alive" : "close");
  if (!enviar(so, cab, strlen(cab), s)) return 0;
  return head || !n || enviar(so, corpo, n, s);
}
static int esperarPronto(Sessao *s) {
  struct timespec ts;
  int ok;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += ESPERA_PREP_MS / 1000;
  pthread_mutex_lock(&s->m);
  while (!s->pronto && !s->falhou && !s->cancel)
    if (pthread_cond_timedwait(&s->c, &s->m, &ts) == ETIMEDOUT) break;
  ok = s->pronto && !s->falhou;
  pthread_mutex_unlock(&s->m);
  return ok;
}
// A generated answer: -2 = stopped (close the connection, nothing sent).
static int entregar(int so, Sessao *s, const char *nome, int e, Resp *r, const char *err, int manter, int head) {
  if (e == -2) { printf("[doador] %s dropped: %s\n", nome, err); fflush(stdout); return 0; }
  if (e < 0) {
    if (!s->cancel) { s->nErro++; snprintf(s->ultErro, sizeof s->ultErro, "%s", err); printf("[doador] %s failed: %s\n", nome, err); fflush(stdout); }
    return responder(so, 502, NULL, "", 0, 0, head, s) && 0;
  }
  return responder(so, 200, r->tipo, r->b, r->n, manter, head, s);
}
// One request. Returns 0 to close the connection.
static int atender(int so, const char *metodo, const char *caminho, int manter) {
  Sessao *s = pegar();
  char nome[128], err[160] = "";
  int head = !strcmp(metodo, "HEAD"), k = -1, i = -1, ok = 1, e;
  size_t lt;
  Resp r;
  double t0 = agoraMs();
  memset(&r, 0, sizeof r);
  if (!s || caminho[0] != '/' || strncmp(caminho + 1, s->tok, (lt = strlen(s->tok))) || caminho[1 + lt] != '/') {
    soltar(s);
    return responder(so, 404, NULL, "", 0, 0, head, NULL) && 0;
  }
  snprintf(nome, sizeof nome, "%s", caminho + 2 + lt);
  if (!esperarPronto(s)) {
    printf("[doador] request %s: the session is not ready (%s)\n", nome, s->motivo[0] ? s->motivo : "timeout");
    fflush(stdout);
    ok = responder(so, 503, NULL, "", 0, 0, head, s) && 0;
    soltar(s); return ok;
  }
  if (!strcmp(nome, "master.m3u8")) {
    Txt t = { 0 };
    plMaster(s, &t);
    ok = t.b && responder(so, 200, "application/vnd.apple.mpegurl", t.b, t.n, manter, head, s);
    printf("[doador] master.m3u8 served (%zu bytes): the converter's video + %d audio rendition(s)\n", t.n, s->nfx);
    fflush(stdout);
    free(t.b);
  } else if (sscanf(nome, "a%d.m3u8", &k) == 1 && strstr(nome, ".m3u8") && !strchr(nome, '_')) {
    Txt t = { 0 };
    if (k < 0 || k >= s->nfx || s->fx[k].modo == M_FORA) { soltar(s); return responder(so, 404, NULL, "", 0, manter, head, s); }
    plAudio(s, &t, k);
    ok = t.b && responder(so, 200, "application/vnd.apple.mpegurl", t.b, t.n, manter, head, s);
    printf("[doador] audio playlist a%d (%s audio %d, %s, %d segments of %.0f s) served\n", k, s->fx[k].fonte ? "donor" : "file",
           s->fx[k].ord, MODO_NOME[s->fx[k].modo], s->naSeg, AUD_SEG_S);
    fflush(stdout);
    free(t.b);
  } else if (sscanf(nome, "a%d_init.mp4", &k) == 1 && strstr(nome, "_init.mp4")) {
    if (k < 0 || k >= s->nfx || s->fx[k].modo != M_MP4) { soltar(s); return responder(so, 404, NULL, "", 0, manter, head, s); }
    pthread_mutex_lock(&s->gm);
    e = gerarAudio(s, k, -1, &r, err, sizeof err, -1, -1);
    pthread_mutex_unlock(&s->gm);
    printf("[doador] a%d init: %s (%zu bytes)\n", k, e < 0 ? err : "ok", r.n);
    fflush(stdout);
    ok = entregar(so, s, nome, e, &r, err, manter, head);
  } else if (sscanf(nome, "a%d_%d.", &k, &i) == 2) {
    long chave = (long)(k + 1) * 1000000L + i;
    if (k < 0 || k >= s->nfx || s->fx[k].modo == M_FORA || i < 0 || i >= s->naSeg) {
      soltar(s); return responder(so, 404, NULL, "", 0, manter, head, s);
    }
    atomic_store(&s->espChave, chave); atomic_fetch_add(&s->espera, 1);
    pthread_mutex_lock(&s->gm);
    atomic_fetch_sub(&s->espera, 1);
    e = gerarAudio(s, k, i, &r, err, sizeof err, so, chave);
    pthread_mutex_unlock(&s->gm);
    ok = entregar(so, s, nome, e, &r, err, manter, head);
  } else ok = responder(so, 404, NULL, "", 0, manter, head, s);
  if (r.b && (agoraMs() - t0) > 4000) { printf("[doador] %s took %.0f ms (the player waited)\n", nome, agoraMs() - t0); fflush(stdout); }
  free(r.b);
  soltar(s);
  return ok;
}
static int temSemCaixa(const char *h, const char *n) {
  size_t k = strlen(n);
  for (; *h; h++) if (!strncasecmp(h, n, k)) return 1;
  return 0;
}
static void *conexao(void *u) {
  int so = (int)(intptr_t)u;
  char b[8192];
  size_t n = 0;
  for (;;) {
    char *fim, metodo[8], caminho[512];
    int manter;
    struct pollfd p = { so, POLLIN, 0 };
    b[n] = 0;
    while (!(fim = strstr(b, "\r\n\r\n"))) {
      ssize_t k;
      if (n >= sizeof b - 1 || poll(&p, 1, 15000) <= 0) goto sair;
      k = recv(so, b + n, sizeof b - 1 - n, 0);
      if (k <= 0) goto sair;
      n += (size_t)k; b[n] = 0;
    }
    if (sscanf(b, "%7s %511s", metodo, caminho) != 2) goto sair;
    { char *q = strchr(caminho, '?'); if (q) *q = 0; }
    manter = !temSemCaixa(b, "\r\nConnection: close");
    if (!atender(so, metodo, caminho, manter) || !manter) goto sair;
    { size_t usado = (size_t)(fim + 4 - b); memmove(b, b + usado, n - usado); n -= usado; }
  }
sair:
  close(so);
  atomic_fetch_sub(&vivos, 1);
  return NULL;
}
static void *aceitar(void *u) {
  (void)u;
  for (;;) {
    int so = accept(ouvinte, NULL, NULL);
    pthread_t t;
    if (so < 0) { if (errno != EINTR) usleep(100000); continue; }
    if (atomic_load(&vivos) >= TBA_CONEXOES) {
      const char *r = "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      send(so, r, strlen(r), MSG_NOSIGNAL); close(so);
      printf("[doador] %d connections open: 503\n", TBA_CONEXOES); fflush(stdout);
      continue;
    }
    { struct timeval tv = { 1, 0 }; setsockopt(so, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv); }
    atomic_fetch_add(&vivos, 1);
    if (pthread_create(&t, NULL, conexao, (void *)(intptr_t)so) == 0) pthread_detach(t);
    else { close(so); atomic_fetch_sub(&vivos, 1); }
  }
  return NULL;
}
static int subirServidor(void) {
  struct sockaddr_in a;
  socklen_t al = sizeof a;
  int um = 1, so;
  pthread_t t;
  if (ouvinte >= 0) return 1;
  so = socket(AF_INET, SOCK_STREAM, 0);
  if (so < 0) return 0;
  setsockopt(so, SOL_SOCKET, SO_REUSEADDR, &um, sizeof um);
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(so, (struct sockaddr *)&a, sizeof a) < 0 || listen(so, 16) < 0 ||
      getsockname(so, (struct sockaddr *)&a, &al) < 0) { close(so); return 0; }
  porta = ntohs(a.sin_port);
  ouvinte = so;
  if (pthread_create(&t, NULL, aceitar, NULL) != 0) { close(so); ouvinte = -1; return 0; }
  pthread_detach(t);
  return 1;
}

// ---- API ----------------------------------------------------------------------------
int doa_disponivel(void) { return avcodec_find_encoder(AV_CODEC_ID_AC3) != NULL; }

int doa_abrir(const DoaAbrir *o, char *out, size_t n) {
  Sessao *s;
  pthread_t t;
  int doador;
  size_t lv;
  if (out && n) out[0] = 0;
  if (!o || !o->video || !*o->video || !o->orig || !*o->orig || !out) return 0;
  doador = o->doador && *o->doador;
  doa_fechar();
  pthread_mutex_lock(&trava);
  if (!subirServidor()) { pthread_mutex_unlock(&trava); printf("[doador] local server did not start\n"); return 0; }
  pthread_mutex_unlock(&trava);
  s = calloc(1, sizeof *s);
  if (!s) return 0;
  atomic_store(&s->refs, 2);   // the API's, the preparation's
  pthread_mutex_init(&s->m, NULL); pthread_cond_init(&s->c, NULL); pthread_mutex_init(&s->gm, NULL); pthread_mutex_init(&s->mm, NULL);
  s->fo[0].k = s->fo[1].k = 1.0;
  s->ppmPedido = o->doador_ppm; s->seguir = o->seguir;
  if (o->doador_ppm) s->fo[1].k = 1.0 + o->doador_ppm / 1e6;
  atomic_store(&s->espChave, -1L);
  s->flacConv = o->flac ? 1 : 0;
  s->audio = o->audio; s->gFaixa = -1; s->gSeg = -1; s->gFonte = -1;
  s->umaConexao = !o->duas_conexoes;
  s->nfo = doador ? 2 : 1;
  // The converter's master (".../m.m3u8") names its media playlist v.m3u8:
  // the TV gets that one straight (a master cannot point at a master).
  s->video = malloc((lv = strlen(o->video)) + 1);
  if (s->video) {
    memcpy(s->video, o->video, lv + 1);
    if (lv > 7 && !strcmp(s->video + lv - 7, "/m.m3u8")) s->video[lv - 6] = 'v';
  }
  s->fo[0].url = strdup(o->orig); s->fo[0].cab = strdup(o->cab ? o->cab : "");
  if (doador) {
    s->fo[1].url = strdup(o->doador); s->fo[1].cab = strdup(o->doador_cab ? o->doador_cab : "");
    snprintf(s->fo[1].nome, sizeof s->fo[1].nome, "%s", o->doador_nome ? o->doador_nome : "");
    s->fo[1].desloc = o->doador_ms / 1000.0; s->doadorMs = o->doador_ms;
    snprintf(s->doadorLingua, sizeof s->doadorLingua, "%s", o->doador_lingua ? o->doador_lingua : "");
    s->doadorFaixa = o->doador_faixa > 0 ? o->doador_faixa : 0;
    snprintf(s->dicaLingua, sizeof s->dicaLingua, "%s", o->dica_lingua ? o->dica_lingua : "");
    snprintf(s->dicaCodec, sizeof s->dicaCodec, "%s", o->dica_codec ? o->dica_codec : "");
    snprintf(s->dicaTitulo, sizeof s->dicaTitulo, "%s", o->dica_titulo ? o->dica_titulo : "");
    s->dicaCanais = o->dica_canais;
    s->doadorPadrao = o->doador_padrao;
  }
  gerarToken(s->tok, 28);
  if (!s->video || !s->fo[0].url || !s->fo[0].cab || (doador && (!s->fo[1].url || !s->fo[1].cab))) {
    atomic_store(&s->refs, 1); soltar(s); return 0;
  }
  pthread_mutex_lock(&trava);
  atual = s;
  pthread_mutex_unlock(&trava);
  if (pthread_create(&t, NULL, preparar, s) != 0) { atomic_fetch_sub(&s->refs, 1); falhar(s, "no thread"); }
  else pthread_detach(t);
  snprintf(out, n, "http://127.0.0.1:%d/%s/master.m3u8", porta, s->tok);
  printf("[doador] session open: the converter's video, file audio %d, FLAC %s%s%s%s; %s; the player opens http://127.0.0.1:%d/<token>/master.m3u8\n",
         o->audio, s->flacConv ? "converted" : "as is", doador ? ", donor \"" : ", no donor", doador ? s->fo[1].nome : "",
         doador ? "\"" : "", s->umaConexao ? "one audio connection" : "a connection per audio source", porta);
  if (doador) printf("[doador] donor: offset %+d ms, starts on %s%s%s\n", o->doador_ms, s->doadorPadrao ? "its track" : "the file's track",
                     s->doadorPadrao && s->doadorLingua[0] ? " in " : "", s->doadorPadrao ? s->doadorLingua : "");
  fflush(stdout);
  return 1;
}
void doa_fechar(void) {
  Sessao *s;
  pthread_mutex_lock(&trava);
  s = atual; atual = NULL;
  pthread_mutex_unlock(&trava);
  if (!s) return;
  s->cancel = 1;
  { int k; for (k = 0; k < TBA_FONTES; k++) if (s->fo[k].eng) dts_engine_interrupt(s->fo[k].eng, 1); }
  pthread_mutex_lock(&s->m); if (s->mEng) dts_engine_cancel(s->mEng); pthread_mutex_unlock(&s->m);
  dts_remux_set_tap(NULL, s, -1);
  pthread_mutex_lock(&s->m); pthread_cond_broadcast(&s->c); pthread_mutex_unlock(&s->m);
  printf("[doador] session closed: %ld audio segments, %lld KB, %ld dropped, %ld failed%s%s\n",
         s->nSeg, s->bytes / 1024, s->nLarg, s->nErro, s->ultErro[0] ? "; last error: " : "", s->ultErro);
  fflush(stdout);
  soltar(s);
}
int doa_ativo(void) { Sessao *s = pegar(); int v = s != NULL; soltar(s); return v; }
int doa_falhou(char *m, size_t n) {
  Sessao *s = pegar();
  int v = 0;
  if (m && n) m[0] = 0;
  if (s) { pthread_mutex_lock(&s->m); v = s->falhou; if (v && m && n) snprintf(m, n, "%s", s->motivo); pthread_mutex_unlock(&s->m); }
  soltar(s);
  return v;
}
int doa_pronto(void) { Sessao *s = pegar(); int v = s && s->pronto; soltar(s); return v; }
int doa_rendicao_de(int ord) {
  Sessao *s = pegar();
  int v = -1, i, k = 0;
  if (s && s->pronto)
    for (i = 0; i < s->nfx; i++) {
      if (s->fx[i].modo == M_FORA) continue;
      if (s->fx[i].chave == ord) { v = k; break; }
      k++;
    }
  soltar(s);
  return v;
}
void doa_definir_audio(int ord) {
  Sessao *s = pegar();
  int i;
  if (s && s->pronto) {
    pthread_mutex_lock(&s->gm);
    for (i = 0; i < s->nfx; i++) if (s->fx[i].chave == ord && s->fx[i].modo != M_FORA) s->audio = i;
    pthread_mutex_unlock(&s->gm);
    if (ord >= DOA_DOADOR(0)) printf("[doador] donor audio %d is the session's track now\n", ord - DOA_DOADOR(0));
    else printf("[doador] audio %d is the session's track now\n", ord);
    fflush(stdout);
  }
  soltar(s);
}
int doa_audio(void) { Sessao *s = pegar(); int v = s && s->pronto && s->audio >= 0 ? s->fx[s->audio].chave : -1; soltar(s); return v; }
int doa_rendicoes(DoaRendicao *out, int max) {
  Sessao *s = pegar();
  int i, k = 0;
  if (s && s->pronto)
    for (i = 0; i < s->nfx && k < max; i++) {
      const Faixa *f = &s->fx[i];
      if (f->modo == M_FORA) continue;
      if (out) {
        DoaRendicao *r = &out[k];
        memset(r, 0, sizeof *r);
        r->chave = f->chave; r->fonte = f->fonte; r->ord = f->ord; r->si = f->si;
        snprintf(r->codecFonte, sizeof r->codecFonte, "%s", f->codec);
        r->canais = f->modo == M_AC3 ? (f->canais > 2 ? 6 : 2) : f->modo == M_AAC ? 2 : f->canais;
        snprintf(r->lingua, sizeof r->lingua, "%s", f->lingua);
        snprintf(r->nome, sizeof r->nome, "%s", f->nome);
        snprintf(r->codec, sizeof r->codec, "%s", f->modo == M_AC3 ? "ac3" : f->modo == M_AAC ? "aac" : f->codec);
      }
      k++;
    }
  soltar(s);
  return k;
}
void doa_desloc(int ms) {
  Sessao *s = pegar();
  if (s && s->pronto && s->nfo > 1 && !s->fo[1].falhou && s->fo[1].ic) {
    double antes;
    pthread_mutex_lock(&s->gm);
    antes = s->fo[1].desloc;
    s->fo[1].desloc = s->fo[0].v0 - s->fo[1].v0 * s->fo[1].k + ms / 1000.0;
    s->doadorMs = ms;
    if (s->gFonte == 1) s->gOk = 0;   // the next segment repositions with the new offset
    pthread_mutex_unlock(&s->gm);
    printf("[doador] donor offset %+d ms (its time %+.3f s = the file's, was %+.3f s): the next segments carry it\n", ms,
           s->fo[1].desloc, antes);
    fflush(stdout);
  }
  soltar(s);
}
int doa_desloc_ms(void) { Sessao *s = pegar(); int v = s ? s->doadorMs : 0; soltar(s); return v; }
int doa_estado(char *m, size_t n) {
  Sessao *s = pegar();
  int v = 0;
  if (m && n) m[0] = 0;
  if (s && s->nfo > 1) {
    if (s->fo[1].falhou) { v = -1; if (m && n) snprintf(m, n, "%s", s->fo[1].motivo); }
    else if (s->pronto) v = 1;
  }
  soltar(s);
  return v;
}
int doa_diferenca(int *corte, int *velocidade, int *ppm) {
  Sessao *s = pegar();
  int v = 0;
  if (s) {
    pthread_mutex_lock(&s->m);
    if (s->difOk && !s->fo[1].falhou) {
      v = 1;
      if (corte) *corte = s->difCorte;
      if (velocidade) *velocidade = s->difVel;
      if (ppm) *ppm = s->difPpm;
    }
    pthread_mutex_unlock(&s->m);
  }
  soltar(s);
  return v;
}
int doa_sondar(const char *url, const char *cab, DoaSonda *out, int max) {
  DtsEngine *e;
  AVFormatContext *ic;
  int i, n = 0;
  if (!url || !*url || !out || max <= 0 || !(e = dts_engine_create())) return -1;
  if (dts_engine_open_demux(e, url, cab ? cab : "") < 0) { dts_engine_destroy(e); return -1; }
  ic = dts_engine_avformat(e);
  if (ic->duration > 0 && ic->duration < (int64_t)DOA_CLIP_S * AV_TIME_BASE) {
    printf("[doador] tracks: %.0f s file, the debrid placeholder clip\n", ic->duration / 1e6);
    fflush(stdout);
    dts_engine_destroy(e);
    return -3;
  }
  for (i = 0; i < (int)ic->nb_streams && n < max; i++) {
    AVStream *st = ic->streams[i];
    AVDictionaryEntry *l, *t;
    Faixa f;
    if (st->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;
    memset(&f, 0, sizeof f);
    f.id = st->codecpar->codec_id; f.canais = st->codecpar->ch_layout.nb_channels;
    if (modoDe(&f, 0) == M_FORA) continue;   // not offered by the server either
    l = av_dict_get(st->metadata, "language", NULL, 0);
    t = av_dict_get(st->metadata, "title", NULL, 0);
    memset(&out[n], 0, sizeof out[n]);
    out[n].si = i; out[n].canais = f.canais;
    snprintf(out[n].codec, sizeof out[n].codec, "%s", avcodec_get_name(f.id));
    snprintf(out[n].lingua, sizeof out[n].lingua, "%s", l && strcmp(l->value, "und") ? l->value : "");
    snprintf(out[n].titulo, sizeof out[n].titulo, "%s", t ? t->value : "");
    n++;
  }
  printf("[doador] tracks of another version: %d audio track(s) offered\n", n);
  fflush(stdout);
  dts_engine_destroy(e);
  return n;
}
void doa_dopasuj(int automatico) {
  Sessao *s = pegar();
  if (s) matchIniciar(s, automatico, 0);
  soltar(s);
}
int doa_dopasuj_resultado(DoaDopasuj *out, int *automatico) {
  Sessao *s = pegar();
  int v = 0;
  if (s) {
    pthread_mutex_lock(&s->m);
    if (s->mNovo) {
      s->mNovo = 0;
      v = s->mEstado == 2 ? 1 : -1;
      if (out) *out = s->mRes;
      if (automatico) *automatico = s->mAuto;
      s->mEstado = 0;
    }
    pthread_mutex_unlock(&s->m);
  }
  soltar(s);
  return v;
}
int doa_doador_saude(int *lento) {
  Sessao *s = pegar();
  int v = 0;
  if (lento) *lento = 0;
  if (s) {
    pthread_mutex_lock(&s->m);
    v = s->dSemResposta == 1; if (v) s->dSemResposta = 2;
    if (lento && s->dLento >= 2 && !s->dLentoDito) { *lento = 1; s->dLentoDito = 1; }
    pthread_mutex_unlock(&s->m);
  }
  soltar(s);
  return v;
}
int doa_dopasuj_ativo(void) { Sessao *s = pegar(); int v = s && s->mEstado == 1; soltar(s); return v; }
int doa_seguir_evento(int *ms) {
  Sessao *s = pegar();
  int v = 0;
  if (s) {
    pthread_mutex_lock(&s->m);
    v = s->segEvento; s->segEvento = 0;
    if (v && ms) *ms = s->segMs;
    pthread_mutex_unlock(&s->m);
  }
  soltar(s);
  return v;
}
void doa_log(void) {
  Sessao *s = pegar();
  if (s) {
    DtsEngineRings an;
    int meus = -1, socks = skContar(&meus), k;
    dts_engine_rings(&an, 0);
    printf("[doador] 10s: %s, audio segments %ld (%lld KB, avg %.0f ms), dropped %ld, failed %ld%s%s\n",
           s->pronto ? "ready" : s->falhou ? "failed" : "preparing", s->nSeg, s->bytes / 1024, s->nSeg ? s->msLer / s->nSeg : 0.0,
           s->nLarg, s->nErro, s->ultErro[0] ? "; last: " : "", s->ultErro);
    // The spike's questions (owner 15/11): connections to the files' hosts
    // (all of Nuvio's, the converter's included, and the TV player's), the
    // readers' memory, and what switching between the sources costs.
    printf("[doador] 10s connections: %d to the watched hosts (%d of them Nuvio's)%s; reader rings alive %d = %lld MiB (peak %d = %lld MiB)",
           socks, meus, socks < 0 ? " (not readable here)" : "", an.alive, an.bytes >> 20, an.peak, an.peak_bytes >> 20);
    for (k = 0; k < s->nfo; k++)
      printf("; %s %s, gave back %ld, took again %ld", k ? "donor" : "file",
             s->fo[k].falhou ? "failed" : !s->fo[k].ic ? "closed" : s->fo[k].conectada ? "connected" : "released",
             s->fo[k].nSoltas, s->fo[k].nVoltas);
    for (k = 0; k < s->nfo; k++)
      if (s->fo[k].msVolta > 0) printf("; %s's last reconnection to its segment out %.0f ms", k ? "donor" : "file", s->fo[k].msVolta);
    if (s->nfo > 1) printf("; donor offset %+d ms", s->doadorMs);
    printf("\n");
    fflush(stdout);
  }
  soltar(s);
}
#endif
