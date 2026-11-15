/* AUDIO FROM ANOTHER SOURCE on the Samsung .tpk (donor, owner 15/11/2026).
 *
 * Plays the picture of one source with audio tracks taken from another source
 * of the same title: a release whose audio the TV plays as it is (E-AC3, AC3)
 * instead of converting the file's DTS/TrueHD, or a dub the file lacks.
 *
 * How (proven on the S95C with TorBox burn-in, branch subtitle-rewrite,
 * src/dts/tbaudio_tpk.c): a local HTTP server on 127.0.0.1 serves the TV a
 * master playlist whose video is the local DTS converter's playlist in
 * video-only mode (dts_tpk_abrir with DTS_REMUX_NO_AUDIO: its segments carry
 * no audio, which the TV needs to play a rendition) and whose audio tracks are
 * HLS renditions, 2 s segments made on request from the playing file and from
 * the donor. The TV lists the renditions as its audio tracks and plays the
 * chosen one in sync with the video.
 *
 * Audio by codec: AAC / AC3 / E-AC3 / Opus / MP3 copied into MPEG-TS; DTS,
 * TrueHD/MLP converted to AC3 (5.1 640 kb/s, 2.0 192 kb/s); FLAC copied into
 * fragmented MP4 (or converted, `flac`).
 *
 * Timeline: the converter writes every packet at its source time +
 * DTS_REMUX_TS_OFFSET_S. The file's audio goes there as it is; the donor's
 * at its own time + (the file's video start - the donor's) + doador_ms.
 *
 * Connections: one audio connection by default. The source the player is not
 * listening to gives its connection back (dts_engine_release: its header and
 * index stay; taking it again is one Range request). The converter holds one
 * more for the picture. Nothing here logs a URL.
 *
 * Only the .tpk built with FFmpeg has the server; elsewhere the calls are
 * stubs (doa_disponivel() == 0). Host test: tests/dts_tpk_servidor.sh. */
#ifndef NV_DOADOR_TPK_H
#define NV_DOADOR_TPK_H
#include <stddef.h>

/* Donor tracks are keyed DOA_DOADOR(k) (k = its audio track, 0-based, in
 * container order); the file's keep their ordinal. */
#define DOA_DOADOR(k) (1000 + (k))
int  doa_disponivel(void);
typedef struct {
  const char *video;          /* the local converter's URL (dts_tpk_abrir in video-only mode) */
  const char *orig, *cab;     /* the playing file and the add-on's raw header lines */
  int audio;                  /* the file's audio track to start on (its ordinal) */
  int flac;                   /* 1: convert FLAC (else copied into fMP4) */
  const char *doador, *doador_cab, *doador_nome;   /* NULL/"": no donor; nome: the source's label (no URL) */
  const char *doador_lingua;  /* the donor track to start on: these languages in order, comma-separated ("": its first) */
  int doador_faixa;           /* the donor's audio stream picked by hand (container stream index + 1; 0 none): first */
  /* Series memory: the donor track picked in the last episode. The donor's
   * track in that language most like it (title, codec, channels) wins over
   * the language list. NULL/"" none. */
  const char *dica_lingua, *dica_codec, *dica_titulo;
  int dica_canais;
  int doador_padrao;          /* 1: start on the donor's track; 0: on `audio` */
  int doador_ms;              /* the donor's offset, ms, positive = later */
  int duas_conexoes;          /* test: both audio sources keep their connections */
} DoaAbrir;
/* New session (replaces any previous one): writes the URL the TV player opens.
 * The sources are read in a thread; the player's requests wait for it. 0 =
 * cannot (no FFmpeg, no server). */
int  doa_abrir(const DoaAbrir *o, char *out, size_t n);
void doa_fechar(void);
int  doa_ativo(void);
int  doa_pronto(void);
/* The session failed to prepare (the playing file did not open): 1 with the reason (no URL). */
int  doa_falhou(char *motivo, size_t n);
/* The renditions in the master's order (= the TV's audio list): count, up to max. */
typedef struct { int chave, fonte, ord, si, canais; char lingua[16], nome[96], codec[24], codecFonte[24]; } DoaRendicao;
/* si: the track's stream index in its container; codecFonte: its codec there
 * (codec is what the rendition carries: "ac3" for a converted DTS). */
int  doa_rendicoes(DoaRendicao *out, int max);
/* The rendition of a key (index in the master's list), -1 not offered. */
int  doa_rendicao_de(int chave);
/* The track the next playlist/segments use (the DEFAULT=YES rendition), and the current one's key. */
void doa_definir_audio(int chave);
int  doa_audio(void);
/* New donor offset (ms, positive = later): the next segments carry it. The app
 * then seeks in place, so the player drops the segments it holds. */
void doa_desloc(int ms);
int  doa_desloc_ms(void);
/* 1 the donor's tracks are offered, 0 no donor (or still preparing), -1 it
 * failed to open (reason in `motivo`, no URL): the file's own tracks go on. */
int  doa_estado(char *motivo, size_t n);
/* A summary line for the 10 s log: segments, connections to the files' hosts,
 * reader memory, which source holds a connection, the last reconnection. */
void doa_log(void);
/* Language comparison for the donor track to start on (the app passes
 * ling_casa: "pl" matches "pol"); NULL or unset compares case-insensitively. */
void doa_definir_casa(int (*casa)(const char *codigo, const char *pref));
/* The donor against the file, once both opened (1; 0 not yet / no donor):
 * corte = the donor's length minus the file's (s, another cut when large),
 * velocidade = 1 when their frame rates differ (a sped-up version), ppm =
 * the donor's speed against the file's from the frame rates (0 unknown). */
int  doa_diferenca(int *corte, int *velocidade, int *ppm);
/* The audio tracks of a file (the audio menu's list for another version):
 * blocking, opens its header. Returns the count, -1 it did not open, -3 the
 * debrid's placeholder clip (under two minutes). */
typedef struct { int si, canais; char codec[24], lingua[16], titulo[64]; } DoaSonda;
int  doa_sondar(const char *url, const char *cab, DoaSonda *out, int max);
#endif
