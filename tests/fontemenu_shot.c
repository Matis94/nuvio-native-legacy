// Captures of the hold-OK source menu ("Use as audio" / "Files of this
// version") on the real source sheet, and checks that it stays off while
// "Audio from another version" is off. Not in the suite (*_shot.sh): GL window.
//   bash tests/fontemenu_shot.sh <dir> [ui-language-index]
#include "../src/streams.c"
#include "ajustes.h"
#include "gfx.h"
#include "text.h"
#include "tex_cache.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint fbo, fboTex;
enum { LW = 1920, LH = 1080 };
static const char *saida;
static unsigned char quadro[LW * LH * 3], rgba[LW * LH * 4];
static Uint32 relogio = 100000;

// GLES2 only guarantees RGBA reads.
static void salvar(const char *id) {
  SDL_Surface *s = SDL_CreateRGBSurface(0, LW, LH, 24, 0xff, 0xff00, 0xff0000, 0);
  char nome[700];
  glFinish(); glBindFramebuffer(GL_FRAMEBUFFER, fbo); glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, LW, LH, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  for (size_t i = 0; i < (size_t)LW * LH; i++) memcpy(quadro + i * 3, rgba + i * 4, 3);
  for (int y = 0; y < LH; y++)
    memcpy((unsigned char *)s->pixels + y * s->pitch, quadro + (size_t)(LH - 1 - y) * LW * 3, (size_t)LW * 3);
  snprintf(nome, sizeof nome, "%s/%s.png", saida, id);
  assert(IMG_SavePNG(s, nome) == 0);
  SDL_FreeSurface(s);
  fprintf(stderr, "captura: %s\n", nome);
}
static void quadros(int n) {
  for (int i = 0; i < n; i++) {
    relogio += 16;
    SDL_PumpEvents(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(8);
    stream_folha_atualizar(1.f / 60, relogio);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
    glClearColor(0.025f, 0.025f, 0.03f, 1); glClear(GL_COLOR_BUFFER_BIT);
    gfx_novo_quadro();
    gfx_cor((GfxRect){ 0, 0, LW, LH }, 0, .20f, .14f, .12f, 1);
    stream_folha_desenhar(relogio);
  }
}
static int linhaDoTeste(const char *trecho) {
  for (int r = 0; r < nOrdem; r++) {
    int i = filtrado(r);
    if (i >= 0 && strstr(lista[i].url, trecho)) return r;
  }
  assert(!"source row not found");
  return -1;
}
static void tecla(SDL_Keycode k, int baixo, int repetir) {
  SDL_Event e;
  memset(&e, 0, sizeof e);
  e.type = baixo ? SDL_KEYDOWN : SDL_KEYUP;
  e.key.keysym.sym = k; e.key.repeat = repetir;
  stream_folha_evento(&e);
}
static void ajustesEscrever(int idioma, int externo) {
  char caminho[700]; FILE *f;
  snprintf(caminho, sizeof caminho, "%s/ajustes.txt", getenv("NUVIO_DADOS"));
  f = fopen(caminho, "w"); assert(f);
  fprintf(f, "idioma %d\nselected_theme 2\nfonteInterface 3\n", idioma);
  if (externo) fprintf(f, "externalAudioLocal 0\n");
  fclose(f);
  ajustes_dir(getenv("NUVIO_DADOS"));
  ajustes_iniciar();
}

static int fakeSonda(const char *url, const char *cab, AudioExtFaixa *out, int max) {
  static const char *const lg[] = { "eng", "pol", "pol", "ces" };
  static const char *const ti[] = { "", "Dubbing", "Lektor", "" };
  (void)url; (void)cab;
  for (int i = 0; i < 4 && i < max; i++) {
    memset(&out[i], 0, sizeof out[i]);
    out[i].stream = i + 1; out[i].canais = i == 0 ? 6 : 2;
    snprintf(out[i].codec, sizeof out[i].codec, "%s", i == 0 ? "eac3" : "ac3");
    snprintf(out[i].idioma, sizeof out[i].idioma, "%s", lg[i]);
    snprintf(out[i].titulo, sizeof out[i].titulo, "%s", ti[i]);
  }
  return 4;
}
static const char *JSON =
  "{\"streams\":["
  "{\"url\":\"https://example.invalid/remux.mkv\",\"name\":\"AIOStreams 2160p\",\"description\":\"2160p REMUX DV · Cached\\n58 GB\\nEnglish\"},"
  "{\"url\":\"https://example.invalid/webdl-pl.mkv\",\"name\":\"AIOStreams 1080p\",\"description\":\"1080p WEB-DL DD+ 5.1 · Cached\\n4.1 GB\\nPolish\"},"
  "{\"url\":\"https://example.invalid/web.avi\",\"name\":\"Torrentio 720p\",\"description\":\"720p · Cached\\nfile.avi\"}"
  "]}";

int main(int argc, char **argv) {
  Stream *v = NULL;
  int count, idioma = argc > 2 ? atoi(argv[2]) : 10;   // 10 = Polish
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
  SDL_Window *w = SDL_CreateWindow("Nuvio: source menu", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN); assert(w);
  SDL_GLContext gl = SDL_GL_CreateContext(w); assert(gl); SDL_GL_SetSwapInterval(0);
  glGenTextures(1, &fboTex); glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");

  // Default: the option is off, and so is the menu (the hold keeps its old job).
  ajustesEscrever(idioma, 0);
  assert(!ajustes_audio_externo());
  count = stream_extrair(JSON, "AIOStreams", &v);
  assert(count == 3);
  stream_definir_alvo("tt0903747:1:1");
  stream_definir_lista(v, count);
  stream_folha_nome("Breaking Bad");
  stream_folha_contexto("S1:E1 · Breaking Bad");
  stream_folha_abrir();
  quadros(40);
  grupo = 1; foco = linhaDoTeste("webdl-pl");
  assert(!fonteAceitaAudio(filtrado(foco)));
  fonteSegurou(filtrado(foco));
  assert(!fmAberto);

  // On: the hold on the Polish WEB-DL opens the menu with "Use as audio".
  ajustesEscrever(idioma, 1);
  assert(ajustes_audio_externo());
  quadros(5);
  grupo = 1; foco = linhaDoTeste("webdl-pl");
  assert(fonteAceitaAudio(filtrado(foco)) && fonteSegurarAceita(filtrado(foco)));
  salvar("fontes-dica");
  // The real gesture: OK down, held past the threshold, then released.
  tecla(SDLK_RETURN, 1, 0);
  for (int i = 0; i < 60 && !fmAberto; i++) quadros(1);
  assert(fmAberto && fmN >= 1 && fmAcoes[0] == FM_AUDIO);
  tecla(SDLK_RETURN, 1, 1);   // key repeat while still held: not a choice
  tecla(SDLK_RETURN, 0, 0);   // release: still open
  assert(fmAberto);
  quadros(30);
  salvar("menu-uzyj");
  // OK: marked, menu closed, the row shows "Audio source".
  tecla(SDLK_RETURN, 1, 0);
  assert(!fmAberto && audioext_marcada(lista[filtrado(foco)].url));
  quadros(30);
  salvar("fontes-oznaczone");
  // Open again: the row now reads "Stop using as audio".
  fonteMenuAbrir(filtrado(foco));
  quadros(30);
  salvar("menu-nie-uzywaj");
  tecla(SDLK_RETURN, 1, 0);
  assert(!audioext_tem_marca());
  // Back closes without acting.
  fonteMenuAbrir(filtrado(foco));
  tecla(SDLK_ESCAPE, 1, 0);
  assert(!fmAberto && !audioext_tem_marca());
  // "Choose audio track…": the version's tracks (read on a thread) as rows;
  // one picked marks the version with that track.
  audioext_definir_sonda(fakeSonda);
  fonteMenuAbrir(filtrado(foco));
  assert(fmN >= 2 && fmAcoes[1] == FM_FAIXAS);
  tecla(SDLK_DOWN, 1, 0); tecla(SDLK_RETURN, 1, 0);
  assert(fmAberto && fmModoFaixas);
  for (int i = 0; i < 200 && fmSondaN == -2; i++) { SDL_Delay(2); quadros(1); }
  assert(fmSondaN == 4);
  quadros(30);
  salvar("menu-sciezki");
  tecla(SDLK_DOWN, 1, 0); tecla(SDLK_DOWN, 1, 0); tecla(SDLK_RETURN, 1, 0);
  assert(!fmAberto && audioext_marcada(lista[filtrado(foco)].url) && audioext_faixa() == 3);
  audioext_limpar();
  // The AVI source is not offered as audio (the converter does not open it).
  foco = linhaDoTeste("web.avi");
  assert(!fonteAceitaAudio(filtrado(foco)));
  free(v);
  puts("PASS: source menu off by default, hold opens it, mark/unmark, AVI refused.");
  return 0;
}
