// Audio from another version of the title: the marked source (audioext.h).
#include "audioext.h"
#include "mediaurl.h"
#include <stdio.h>
#include <string.h>

static char alvoAtual[64], alvoMarca[64];
static char urlMarca[NV_MEDIA_URL_MAX], cabMarca[512], nomeMarca[192];
static int offsetMs, tocando;
static unsigned offsetGeracao;
static char falha[160], idiomaMarca[8];
static unsigned pedidoGeracao;
static int prefSoAudio, prefSerie = 1;
// SAFETY NET (owner, 07/11): a start with this mark began and the other
// version's audio never played (it failed, hung, or the person backed out
// while it loaded). The next start of the title then plays the file's own
// audio instead of trying the same thing again.
static int pendente, voltouPadrao;
// One remembered series at a time (test build, not saved).
static struct { char serie[64], episodio[64]; AudioExtIdent id; int offset, ativo; } memo;
static void serieDe(const char *alvo, char *out, size_t tam) {
  const char *c = alvo ? strchr(alvo, ':') : NULL;
  snprintf(out, tam, "%.*s", c ? (int)(c - alvo) : 0, c ? alvo : "");
}

void audioext_limpar(void) {
  urlMarca[0] = cabMarca[0] = nomeMarca[0] = alvoMarca[0] = idiomaMarca[0] = 0;
  offsetMs = 0; offsetGeracao++;
  pendente = 0;
}
// "tt123" and "tt123:1:2" are the same title: the detail page stamps the raw
// id before the episode list arrives (addons.c). Another episode is not.
static int mesmoAlvo(const char *a, const char *b) {
  size_t na = strlen(a), nb = strlen(b);
  if (!strcmp(a, b)) return 1;
  if (na < nb) return !strncmp(a, b, na) && b[na] == ':' && !strchr(a, ':');
  if (nb < na) return !strncmp(a, b, nb) && a[nb] == ':' && !strchr(b, ':');
  return 0;
}
void audioext_alvo(const char *alvo) {
  snprintf(alvoAtual, sizeof alvoAtual, "%s", alvo ? alvo : "");
  if (urlMarca[0] && alvoAtual[0] && !mesmoAlvo(alvoAtual, alvoMarca)) {
    printf("[ext-audio] mark cleared: another title or episode (%s)\n", alvoAtual);
    fflush(stdout);
    audioext_limpar();
  }
}
int audioext_alternar(const char *url, const char *cabecalhos, const char *nome) {
  if (!url || !*url) return -1;
  if (audioext_marcada(url)) {
    audioext_limpar();
    printf("[ext-audio] mark cleared by the user\n"); fflush(stdout);
    return 0;
  }
  snprintf(urlMarca, sizeof urlMarca, "%s", url);
  snprintf(cabMarca, sizeof cabMarca, "%s", cabecalhos ? cabecalhos : "");
  snprintf(nomeMarca, sizeof nomeMarca, "%s", nome ? nome : "");
  snprintf(alvoMarca, sizeof alvoMarca, "%s", alvoAtual);
  idiomaMarca[0] = 0;
  offsetMs = 0; offsetGeracao++;
  pendente = 0;
  // The name is the add-on's label, never the URL (signed links stay out of logs).
  printf("[ext-audio] audio source marked: %.80s (target %s)\n", nomeMarca, alvoMarca);
  fflush(stdout);
  return 1;
}
void audioext_escolher(const char *url, const char *cabecalhos, const char *nome, const char *idioma) {
  if (!url || !*url) return;
  if (!audioext_marcada(url)) {
    snprintf(urlMarca, sizeof urlMarca, "%s", url);
    snprintf(cabMarca, sizeof cabMarca, "%s", cabecalhos ? cabecalhos : "");
    snprintf(nomeMarca, sizeof nomeMarca, "%s", nome ? nome : "");
    snprintf(alvoMarca, sizeof alvoMarca, "%s", alvoAtual);
    offsetMs = 0; offsetGeracao++;
  }
  snprintf(idiomaMarca, sizeof idiomaMarca, "%s", idioma ? idioma : "");
  pedidoGeracao++;
  pendente = 1;   // the player reopens with it now
  printf("[ext-audio] chosen in the audio menu: %.80s lang=%s (target %s)\n", nomeMarca, idiomaMarca, alvoMarca);
  fflush(stdout);
}
void audioext_original(void) {
  audioext_limpar();
  audioext_serie_esquecer();
  pedidoGeracao++;
  printf("[ext-audio] back to this file's own audio\n"); fflush(stdout);
}
unsigned audioext_pedido_geracao(void) { return pedidoGeracao; }
int audioext_tem_marca(void) { return urlMarca[0] != 0; }
const char *audioext_idioma(void) { return idiomaMarca; }
int audioext_marcada(const char *url) {
  return urlMarca[0] && url && !strcmp(url, urlMarca);
}
int audioext_pedido(const char *urlVideo) {
  return urlMarca[0] && mesmoAlvo(alvoAtual, alvoMarca) && urlVideo && *urlVideo &&
         strcmp(urlVideo, urlMarca) != 0;
}
const char *audioext_url(void) { return urlMarca; }
const char *audioext_cabecalhos(void) { return cabMarca; }
const char *audioext_nome(void) { return nomeMarca; }
int audioext_offset_ms(void) { return offsetMs; }
int audioext_offset_somar(int ms) {
  offsetMs += ms;
  if (offsetMs > 10000) offsetMs = 10000;
  if (offsetMs < -10000) offsetMs = -10000;
  offsetGeracao++;
  { char sr[64]; serieDe(alvoMarca, sr, sizeof sr);
    if (memo.ativo && sr[0] && !strcmp(sr, memo.serie)) memo.offset = offsetMs; }
  return offsetMs;
}
unsigned audioext_offset_geracao(void) { return offsetGeracao; }
void audioext_tocando_definir(int sim) {
  tocando = sim != 0;
  if (tocando) pendente = 0;
}
void audioext_inicio_reproducao(const char *urlVideo) {
  if (!audioext_pedido(urlVideo)) return;
  if (pendente) {
    printf("[ext-audio] the last start with \"%.80s\" never played its audio: this file's own audio now\n", nomeMarca);
    fflush(stdout);
    audioext_limpar();
    voltouPadrao = 1;
    return;
  }
  pendente = 1;
}
int audioext_voltou_padrao_consumir(void) {
  int v = voltouPadrao;
  voltouPadrao = 0;
  return v;
}
int audioext_tocando(void) { return tocando; }
void audioext_falhou(const char *motivo) {
  snprintf(falha, sizeof falha, "%s", motivo && *motivo ? motivo : "unknown error");
  // The session goes back to the file's own audio, and so does the next start:
  // the mark goes away (it can be picked again in the audio menu).
  audioext_limpar();
}
int audioext_falha_consumir(char *dst, unsigned tam) {
  if (!falha[0]) return 0;
  if (dst && tam) snprintf(dst, tam, "%s", falha);
  falha[0] = 0;
  return 1;
}

void audioext_preferencias(int soAudio, int serie) {
  prefSoAudio = soAudio != 0; prefSerie = serie != 0;
  if (!prefSerie) memo.ativo = 0;
}
int audioext_so_audio(void) { return prefSoAudio; }
void audioext_lembrar_serie(const AudioExtIdent *id) {
  char sr[64];
  serieDe(alvoMarca, sr, sizeof sr);
  if (!prefSerie || !id || !sr[0]) return;
  memset(&memo, 0, sizeof memo);
  snprintf(memo.serie, sizeof memo.serie, "%s", sr);
  snprintf(memo.episodio, sizeof memo.episodio, "%s", alvoMarca);
  memo.id = *id; memo.offset = offsetMs; memo.ativo = 1;
  printf("[ext-audio] series memory: group=%s addon=%.40s lang=%s %dp codec=%d\n", memo.id.grupo[0] ? memo.id.grupo : "-",
         memo.id.addon, memo.id.idioma, memo.id.altura, memo.id.codec);
  fflush(stdout);
}
int audioext_serie_procura(const char *alvo, AudioExtIdent *id) {
  char sr[64];
  if (!prefSerie || !memo.ativo || !alvo || !*alvo) return 0;
  serieDe(alvo, sr, sizeof sr);
  if (!sr[0] || strcmp(sr, memo.serie) || !strcmp(alvo, memo.episodio)) return 0;
  if (urlMarca[0] && mesmoAlvo(alvo, alvoMarca)) return 0;   /* already chosen here */
  if (id) *id = memo.id;
  return 1;
}
void audioext_serie_achou(const char *url, const char *cabecalhos, const char *nome) {
  if (!url || !*url) return;
  snprintf(urlMarca, sizeof urlMarca, "%s", url);
  snprintf(cabMarca, sizeof cabMarca, "%s", cabecalhos ? cabecalhos : "");
  snprintf(nomeMarca, sizeof nomeMarca, "%s", nome ? nome : "");
  snprintf(alvoMarca, sizeof alvoMarca, "%s", alvoAtual);
  snprintf(idiomaMarca, sizeof idiomaMarca, "%s", memo.id.idioma);
  snprintf(memo.episodio, sizeof memo.episodio, "%s", alvoAtual);
  offsetMs = memo.offset; offsetGeracao++;
  pedidoGeracao++;
  printf("[ext-audio] series memory: this episode uses %.80s, offset %d ms\n", nomeMarca, offsetMs);
  fflush(stdout);
}
void audioext_serie_esquecer(void) { memo.ativo = 0; }
