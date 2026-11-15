#ifndef NV_AUDIOEXT_H
#define NV_AUDIOEXT_H
// AUDIO FROM ANOTHER SOURCE (donor), test build on the Samsung .tpk.
//
// In the source sheet the BLUE key on a source marks it as the audio source:
// the next source played for the same title/episode keeps its own picture and
// takes its audio tracks from the marked one (an E-AC3 release instead of
// converting the file's DTS, or a dub the file lacks). The .tpk plays it over
// the donor server (src/dts/doador_tpk.h) and the converter in video-only
// mode (video_tpk.c). BLUE on the marked source again clears it. While it
// plays, CH+/CH- move the donor's audio 50 ms later/earlier.
//
// Same state as the webOS test build (Matis94/nuvio-native-legacy,
// external-audio-test). Main thread only. Nothing is saved: it lasts until
// another title or episode opens its source list, or the app closes.

// The source list now belongs to this target ("tt123", "tt123:1:2"). A
// different target than the marked one forgets the mark.
void audioext_alvo(const char *alvo);
// BLUE on a source of the current target: marks it, or clears the mark when it
// already is the marked one. 1 = marked now, 0 = cleared, -1 = refused (no URL).
int audioext_alternar(const char *url, const char *cabecalhos, const char *nome);
void audioext_limpar(void);
// The source with this URL is the marked one (for the sheet's label).
int audioext_marcada(const char *url);
// 1 when a mark exists for the current target and it is not the video itself.
int audioext_pedido(const char *urlVideo);
const char *audioext_url(void);
const char *audioext_cabecalhos(void);
const char *audioext_nome(void);
// Offset of the donor's audio in ms (positive = later), kept with the mark.
int audioext_offset_ms(void);
// Adds `ms`, bounded to +-10 s; returns the new offset.
int audioext_offset_somar(int ms);
// Bumped by every offset change; the player applies the latest one.
unsigned audioext_offset_geracao(void);
// The player says whether the session on screen plays the donor's tracks (the
// player screen uses it for CH+/CH-), and why it could not.
void audioext_tocando_definir(int sim);
int audioext_tocando(void);
void audioext_falhou(const char *motivo);
// Once: the last failure ("" none), for a toast on the player screen.
int audioext_falha_consumir(char *dst, unsigned tam);
#endif
