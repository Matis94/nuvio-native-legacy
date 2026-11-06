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
// From the audio menu: back to this file's own audio, reopened the same way.
void audioext_original(void);
unsigned audioext_pedido_geracao(void);
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
#endif
