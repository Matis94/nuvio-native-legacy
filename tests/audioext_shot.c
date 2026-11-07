// Captures of "From another version" in the audio sheet (audioext.h): the
// candidates grouped by language (Polish UI and Polish preferred audio), and
// the sheet while such audio plays (Audio sync row, this file's own audio).
// Real player/sheet draw path in a hidden GL window, no network:
//   bash tests/audioext_shot.sh <dir>
#include "catalogo.h"
#include "player.h"
#include "ajustes.h"
#include "faixas.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include "episodios.h"
#include "streams.h"
#include "plrilha.h"
#include "video.h"
#include "audioext.h"
#include "fontefiltro.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static GLuint fbo, fboTex;
static int LW = 1920, LH = 1080;
static const char *saida;
static unsigned char quadro[1920 * 1080 * 3], rgba[1920 * 1080 * 4];

// GLES2 only guarantees RGBA reads.
static void ler(void) {
  glFinish(); glBindFramebuffer(GL_FRAMEBUFFER, fbo); glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, LW, LH, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  for (size_t i = 0; i < (size_t)LW * LH; i++) memcpy(quadro + i * 3, rgba + i * 4, 3);
}
static void salvar(const char *id) {
  SDL_Surface *s = SDL_CreateRGBSurface(0, LW, LH, 24, 0xff, 0xff00, 0xff0000, 0);
  char nome[700];
  ler();
  for (int y = 0; y < LH; y++)
    memcpy((unsigned char *)s->pixels + y * s->pitch, quadro + (size_t)(LH - 1 - y) * LW * 3, (size_t)LW * 3);
  snprintf(nome, sizeof nome, "%s/%s.bmp", saida, id);
  assert(SDL_SaveBMP(s, nome) == 0);
  SDL_FreeSurface(s);
  fprintf(stderr, "captura: %s\n", nome);
}
static Uint32 relogio = 100000;
static void quadros(int n) {
  for (int i = 0; i < n; i++) {
    relogio += 16;
    SDL_PumpEvents(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(8);
    player_atualizar(1.f / 60, relogio);
    episodios_atualizar(1.f / 60);
    stream_folha_atualizar(1.f / 60, relogio); faixas_atualizar(1.f / 60, relogio);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
    glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    player_desenhar(relogio);
    episodios_desenhar();
    stream_folha_desenhar(relogio);
    faixas_desenhar(relogio);
    plrilha_desenhar(relogio);
  }
}
static void tecla(SDL_Keycode k) {
  SDL_Event ev; memset(&ev, 0, sizeof ev);
  ev.type = SDL_KEYDOWN; ev.key.keysym.sym = k; faixas_evento(&ev);
  quadros(3);
}
static void faixa(VideoFaixa *f, int numero, const char *rot, const char *idioma) {
  memset(f, 0, sizeof *f);
  f->numero = numero; f->ordinalMkv = -1;
  snprintf(f->rotulo, sizeof f->rotulo, "%s", rot);
  snprintf(f->idioma, sizeof f->idioma, "%s", idioma);
}
static VideoSimulacao sim;
static void estado(void) {
  video_simular(&sim);
  player_shot_estado(relogio, 1312.0f, 7380.0f, 1, 0, 0, 0);
  player_shot_esconder();
}
static uint64_t bit(int cat, const char *nome) {
  for (int i = 0; i < fontefiltro_opcoes_n(cat); i++)
    if (!strcmp(fontefiltro_opcao(cat, i), nome)) return fontefiltro_bit(cat, i);
  assert(!"filter option");
  return 0;
}
static void fonte(Stream *s, const char *rot, const char *url, const char *langs, const char *audio,
                  const char *canais, const char *qual, int altura, double gb, int cache) {
  memset(s, 0, sizeof *s);
  snprintf(s->rotulo, sizeof s->rotulo, "%s", rot);
  snprintf(s->provedor, sizeof s->provedor, "AIOStreams");
  snprintf(s->url, sizeof s->url, "%s", url);
  snprintf(s->audioIdiomas, sizeof s->audioIdiomas, "%s", langs);
  s->filtro.valores[FONTEFILTRO_AUDIO] = bit(FONTEFILTRO_AUDIO, audio);
  s->filtro.valores[FONTEFILTRO_CANAIS] = bit(FONTEFILTRO_CANAIS, canais);
  s->filtro.valores[FONTEFILTRO_QUALIDADE] = bit(FONTEFILTRO_QUALIDADE, qual);
  s->altura = altura; s->tamanhoBytes = (uint64_t)(gb * 1e9); s->fileIdx = -1;
  s->emCache = cache > 0; s->foraCache = cache < 0; s->filtroLido = 1;
}

int main(int argc, char **argv) {
  saida = argc > 1 ? argv[1] : "/tmp";
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
#ifdef __APPLE__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
  SDL_Window *w = SDL_CreateWindow("Nuvio: external audio", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  SDL_GLContext gl = SDL_GL_CreateContext(w); assert(gl); SDL_GL_SetSwapInterval(0);
  glGenTextures(1, &fboTex); glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  { char caminho[700]; FILE *f;
    snprintf(caminho, sizeof caminho, "%s/ajustes.txt", getenv("NUVIO_DADOS"));
    f = fopen(caminho, "w"); assert(f);
    fprintf(f, "idioma %d\nselected_theme 2\nfonteInterface 3\nexternalAudioLocal 0\n", argc > 2 ? atoi(argv[2]) : 10);   // 10 = Polish; the option is off by default
    fclose(f);
    ajustes_dir(getenv("NUVIO_DADOS")); }
  ajustes_iniciar();
  assert(ajustes_audio_externo() && ajustes_audio_externo_passo() == 50);
  { struct tm lt; time_t t = time(NULL);
    localtime_r(&t, &lt); lt.tm_hour = 21; lt.tm_min = 4; lt.tm_sec = 0;
    plrilha_shot_hora(mktime(&lt)); }
  { CatItem c; memset(&c, 0, sizeof c);
    snprintf(c.tipo, sizeof c.tipo, "movie");
    snprintf(c.titulo, sizeof c.titulo, "A Noite dos Espelhos");
    snprintf(c.imdb, sizeof c.imdb, "tt0000001");
    snprintf(c.backdrop, sizeof c.backdrop, "deploy/app/art/19.jpg");
    cat_definir(&c, 1); }

  // The source list of the title: the remux playing, and other versions.
  static Stream lista[9];
  fonte(&lista[0], "Remux 2160p DV", "https://example.invalid/remux.mkv", "en", "TrueHD", "7.1", "BluRay REMUX", 2160, 78.4, 1);
  fonte(&lista[1], "WEB-DL PL", "https://example.invalid/webdl-pl.mkv", "pl,en", "DD+", "5.1", "WEB-DL", 1080, 6.1, 1);
  fonte(&lista[2], "WEB-DL PL 720", "https://example.invalid/webdl-pl-720.mkv", "pl", "DD", "5.1", "WEB-DL", 720, 2.9, 1);
  fonte(&lista[3], "Remux PL", "https://example.invalid/remux-pl.mkv", "pl", "DD+", "5.1", "BluRay REMUX", 2160, 64.0, 0);
  fonte(&lista[4], "WEB-DL PL uncached", "https://example.invalid/webdl-pl-unc.mkv", "pl", "DD+", "5.1", "WEB-DL", 1080, 5.4, -1);
  fonte(&lista[5], "WEB-DL CZ", "https://example.invalid/webdl-cz.mkv", "cs", "DD+", "5.1", "WEB-DL", 1080, 5.2, 1);
  fonte(&lista[6], "WEBRip DE", "https://example.invalid/webrip-de.mkv", "de", "AAC", "2.0", "WEBRip", 1080, 2.1, 0);
  fonte(&lista[7], "BluRay PL DTS Extended", "https://example.invalid/bd-pl-dts.mkv", "pl", "DTS-HD MA", "5.1", "BluRay", 1080, 14.0, 1);
  fonte(&lista[8], "WEB-DL ES", "https://example.invalid/webdl-es.mkv", "es", "DD+", "5.1", "WEB-DL", 1080, 4.8, 1);
  /* Cut names: "Extended" is another cut than an unnamed (theatrical) one. */
  assert(!strcmp(stream_edicao(&lista[7]), "Extended") && !stream_edicao(&lista[0]));
  assert(stream_outro_corte(&lista[7], &lista[0]) && !stream_outro_corte(&lista[6], &lista[0]));
  { Stream dc = lista[0]; snprintf(dc.rotulo, sizeof dc.rotulo, "Movie.2001.Directors.Cut.1080p");
    assert(!strcmp(stream_edicao(&dc), "Director's Cut")); }
  stream_definir_alvo("tt0000001");
  stream_definir_lista(lista, 9);
  stream_definir_atual(0);
  assert(stream_lista_do_alvo_atual());

  player_abrir(0, NULL);
  player_erro_fonte(); player_limpar_erro_fonte();
  player_shot_video(1);
  memset(&sim, 0, sizeof sim);
  sim.largura = 3840; sim.altura = 2160; sim.pronto = 1; sim.pos = 1312.0; sim.duracao = 7380.0;
  sim.nAudio = 2;
  faixa(&sim.audio[0], 1, "Angielski  \xc2\xb7  TrueHD Atmos 7.1", "en");
  faixa(&sim.audio[1], 2, "Angielski  \xc2\xb7  AC3 5.1", "en");
  sim.nLeg = 0; sim.legAtual = -1;
  estado(); quadros(20);

  // 1. The audio sheet: the file's two tracks, then the other versions,
  // Polish first, then the rest by language.
  faixas_abrir_em(0); estado(); quadros(40);
  fprintf(stderr, "sheet open=%d player=%d tracks=%d\n", faixas_aberta(), player_aberto(), video_n_audio());
  salvar("audio-other-versions");
  // 2. Down to the first other version, the focused row.
  tecla(SDLK_DOWN); tecla(SDLK_DOWN); estado(); quadros(20);
  salvar("audio-other-versions-focus");
  // OK picks it: the request reaches the player (audioext_pedido_geracao).
  { unsigned g = audioext_pedido_geracao();
    tecla(SDLK_RETURN);
    assert(audioext_pedido_geracao() == g + 1 && audioext_tem_marca());
    assert(!strcmp(audioext_url(), "https://example.invalid/webdl-pl.mkv")); }
  quadros(30);
  // 2b. While it opens: the title's art and the loading island, not black.
  tecla(SDLK_ESCAPE);
  audioext_carregando_definir(1);
  estado(); quadros(40);
  salvar("audio-other-version-loading");
  audioext_carregando_definir(0);

  // 3. Playing it: the converter lists the other file's track; Audio sync
  // and this file's own audio come right after it.
  audioext_tocando_definir(1);
  sim.nAudio = 1;
  faixa(&sim.audio[0], 1, "Polski  \xc2\xb7  E-AC3 5.1", "pl");
  faixas_abrir_em(0); estado(); quadros(40);
  tecla(SDLK_DOWN);
  tecla(SDLK_RIGHT); tecla(SDLK_RIGHT); tecla(SDLK_RIGHT);
  assert(audioext_offset_ms() == 150);
  estado(); quadros(20);
  salvar("audio-other-version-playing-sync");
  // 3b. OK: the precise editor (min : s : ms). Digits fill seconds then ms;
  // the other version is 100 s longer, so the lengths suggest -100 s.
  audioext_diferenca(100, 0);
  tecla(SDLK_RETURN); estado(); quadros(20);
  salvar("audio-sync-editor");
  tecla(SDLK_1); tecla(SDLK_4); tecla(SDLK_0); tecla(SDLK_5); tecla(SDLK_0);
  fprintf(stderr, "typed: %d ms\n", audioext_offset_ms());
  assert(audioext_offset_ms() == 14050);
  tecla(SDLK_LEFT); tecla(SDLK_LEFT); tecla(SDLK_UP);   /* minutes +1 */
  assert(audioext_offset_ms() == 74050);
  estado(); quadros(10);
  salvar("audio-sync-editor-typed");
  tecla(SDLK_RIGHT); tecla(SDLK_RIGHT); tecla(SDLK_RIGHT); tecla(SDLK_RIGHT);   /* -> Use the suggestion */
  tecla(SDLK_RETURN);
  assert(audioext_offset_ms() == -100000);
  estado(); quadros(10);
  salvar("audio-sync-editor-hint");
  tecla(SDLK_LEFT); tecla(SDLK_RETURN);   /* Reset */
  assert(audioext_offset_ms() == 0);
  tecla(SDLK_ESCAPE); assert(faixas_aberta());   /* Back closes the editor only */
  estado(); quadros(10);
  tecla(SDLK_DOWN); estado(); quadros(10);
  salvar("audio-other-version-match");
  tecla(SDLK_DOWN); estado(); quadros(10);
  { unsigned g = audioext_pedido_geracao();
    tecla(SDLK_RETURN);
    assert(audioext_pedido_geracao() == g + 1 && !audioext_tem_marca()); }
  fprintf(stderr, "audioext_shot: ok\n");
  return 0;
}
