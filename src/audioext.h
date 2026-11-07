#ifndef NV_AUDIOEXT_H
#define NV_AUDIOEXT_H
// AUDIO FROM ANOTHER VERSION OF THE TITLE (test build, external-audio-test).
//
// In the source sheet the BLUE key on a source marks it as the audio source:
// the next source played for the same title/episode keeps its own picture and
// takes the audio of the marked one (a WEB-DL dub over a remux, for example).
// The converter reads only the audio of the marked file, over its own small
// ring (dts_engine_external_audio). BLUE on the marked source again clears it.
// While it plays, CH+/CH- move the audio 50 ms later/earlier.
//
// Main thread only. Nothing is saved: it lasts until another title or episode
// opens its source list, or the app closes.

// The source list now belongs to this target ("tt123", "tt123:1:2"). A
// different target than the marked one forgets the mark.
void audioext_alvo(const char *alvo);
// BLUE on a source of the current target: marks it, or clears the mark when it
// already is the marked one. 1 = marked now, 0 = cleared, -1 = refused (no URL).
int audioext_alternar(const char *url, const char *cabecalhos, const char *nome);
void audioext_limpar(void);
// From the audio menu while playing: use this source's audio now (`idioma` is
// the language it was listed under, tried first in that file). The webOS
// player reopens the converter at the current position (audioext_pedido_geracao).
void audioext_escolher(const char *url, const char *cabecalhos, const char *nome, const char *idioma);
// SERIES MEMORY (Settings > Remember for the next episodes): what identifies
// the chosen version across episodes. The add-on's binge group when it sends
// one ("the same release"), else add-on + language + resolution + codec.
typedef struct {
  char grupo[128], addon[96], idioma[8];
  int altura, codec;
} AudioExtIdent;
// Called with the identity right after audioext_escolher, in a series
// episode ("tt123:1:2"): the next episodes look for that version.
void audioext_lembrar_serie(const AudioExtIdent *id);
// The episode on screen ("tt123:1:3") belongs to the remembered series and
// has no audio chosen yet: the source list may pick the version (streams.c).
int audioext_serie_procura(const char *alvo, AudioExtIdent *id);
// streams.c found the remembered version in this episode's list: mark it with
// the remembered sync, as if chosen in the menu.
void audioext_serie_achou(const char *url, const char *cabecalhos, const char *nome);
void audioext_serie_esquecer(void);
// Preferences the UI copies here every frame (the player has no settings).
void audioext_preferencias(int soAudio, int serie);
int audioext_so_audio(void);
// From the audio menu: back to this file's own audio, reopened the same way.
void audioext_original(void);
unsigned audioext_pedido_geracao(void);
// From the audio menu while the other version plays: its audio stream
// `stream` (demux index in that file) instead of the language pick; the webOS
// player reopens with it. Forgotten with the mark. -1 = by language.
void audioext_escolher_faixa(int stream);
int audioext_faixa(void);
int audioext_tem_marca(void);
const char *audioext_idioma(void);
// The source with this URL is the marked one (for the sheet's label).
int audioext_marcada(const char *url);
// 1 when a mark exists for the current target and it is not the video itself.
int audioext_pedido(const char *urlVideo);
const char *audioext_url(void);
const char *audioext_cabecalhos(void);
const char *audioext_nome(void);
// Offset of the external audio in ms (positive = later), kept with the mark.
int audioext_offset_ms(void);
// Adds `ms`, bounded to +-10 s; returns the new offset.
int audioext_offset_somar(int ms);
// Bumped by every offset change; the webOS player applies the latest one.
unsigned audioext_offset_geracao(void);
// The webOS player says whether the session on screen plays the external
// audio (the player screen uses it for CH+/CH-), and why it could not.
void audioext_tocando_definir(int sim);
int audioext_tocando(void);
void audioext_falhou(const char *motivo);
// Once: the last failure ("" none), for a toast on the player screen.
int audioext_falha_consumir(char *dst, unsigned tam);
// A failure clears the mark. The webOS player calls this when a playback
// starts: a start with the mark that never reached audioext_tocando_definir(1)
// (it hung, or the person backed out while it loaded) makes this next start
// clear the mark and play the file's own audio.
void audioext_inicio_reproducao(const char *urlVideo);
// Once: the last start fell back to the file's own audio (player toast).
int audioext_voltou_padrao_consumir(void);
#endif
