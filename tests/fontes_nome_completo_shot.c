// Shot of the sources sheet in "Texto das fontes = Do addon" mode, fed by real
// Stremio JSON through stream_extrair (multi-line description + emojis).
//   bash tests/fonte_addon_texto_shot.sh <dir>
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
#include "badges.h"
#include "idioma.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static GLuint fbo, fboTex;
enum { LW = 1920, LH = 1080 };
static const char *saida;
static unsigned char quadro[LW * LH * 4];
static int noPlayer;
static Uint32 relogio = 100000;

static void salvar(const char *id) {
  SDL_Surface *s = SDL_CreateRGBSurface(0, LW, LH, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
  char nome[700];
  glFinish(); glBindFramebuffer(GL_FRAMEBUFFER, fbo); glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, LW, LH, GL_RGBA, GL_UNSIGNED_BYTE, quadro);
  for (int y = 0; y < LH; y++)
    memcpy((unsigned char *)s->pixels + y * s->pitch, quadro + (size_t)(LH - 1 - y) * LW * 4, (size_t)LW * 4);
  snprintf(nome, sizeof nome, "%s-%s.png", saida, id);
  assert(IMG_SavePNG(s, nome) == 0);
  SDL_FreeSurface(s);
  printf("captura: %s\n", nome);
}

static void quadros(int n) {
  for (int i = 0; i < n; i++) {
    relogio += 16;
    SDL_PumpEvents(); txt_novo_quadro(); tex_novo_quadro(); tex_bombear(8);
    if (noPlayer) player_atualizar(1.f / 60, relogio);
    stream_folha_atualizar(1.f / 60, relogio);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); glViewport(0, 0, LW, LH);
    glClearColor(0.025f, 0.025f, 0.03f, 1); glClear(GL_COLOR_BUFFER_BIT);
    gfx_novo_quadro();
    if (noPlayer) player_desenhar(relogio);
    else {
      gfx_cor((GfxRect){ 0, 0, LW, LH }, 0, .26f, .17f, .12f, 1);
      gfx_cor((GfxRect){ 0, 0, LW, 360 }, 0, .55f, .36f, .22f, 1);
    }
    stream_folha_desenhar(relogio);
    if (noPlayer) plrilha_desenhar(relogio);
  }
}
static void tecla(SDL_Keycode k) {
  SDL_Event e; memset(&e, 0, sizeof e);
  e.type = SDL_KEYDOWN; e.key.keysym.sym = k;
  stream_folha_evento(&e);
  quadros(2);
}


// Real add-on JSON, parsed by the app's own parser, shown with
// Ajustes > Texto das fontes = "Do addon" (fonteTextoLocal 1).
static const char *JSON = "{\"streams\": [{\"url\": \"https://x.invalid/1.mkv\", \"name\": \"AIOStreams 1080p\", \"description\": \"💾 12 GB | 18 Mbps\\n🔊 DDP 5.1\\n🌐 EN | PL\\n⚙ Cached\\n🏷 WEB-DL\\n📦 Przykładowy.Film.Z.Bardzo.Długą.Nazwą.2026.1080p.WEB-DL.DDP5.1.Atmos.H.264-WERSJA-REŻYSERSKA-PEŁNA-NAZWA-PLIKU-GROUP.mkv\", \"behaviorHints\": {\"filename\": \"Przykładowy.Film.Z.Bardzo.Długą.Nazwą.2026.1080p.WEB-DL.DDP5.1.Atmos.H.264-WERSJA-REŻYSERSKA-PEŁNA-NAZWA-PLIKU-GROUP.mkv\"}}, {\"url\": \"https://x.invalid/2.mkv\", \"name\": \"AIOStreams 1080p\", \"description\": \"💾 12 GB | 18 Mbps\\n🔊 DDP 5.1\\n🌐 EN | PL\\n⚙ Cached\\n🏷 WEB-DL\\n📦 Przykładowy.Film.Z.Bardzo.Długą.Nazwą.2026.1080p.WEB-DL.DDP5.1.Atmos.H.264-WERSJA-REŻYSERSKA-PEŁNA-NAZWA-PLIKU-GROUP.mkv\", \"behaviorHints\": {\"filename\": \"Przykładowy.Film.Z.Bardzo.Długą.Nazwą.2026.1080p.WEB-DL.DDP5.1.Atmos.H.264-WERSJA-REŻYSERSKA-PEŁNA-NAZWA-PLIKU-GROUP.mkv\"}}, {\"url\": \"https://x.invalid/3.mkv\", \"name\": \"AIOStreams 1080p\", \"description\": \"💾 12 GB | 18 Mbps\\n🔊 DDP 5.1\\n🌐 EN | PL\\n⚙ Cached\\n🏷 WEB-DL\\n📦 Przykładowy.Film.Z.Bardzo.Długą.Nazwą.2026.1080p.WEB-DL.DDP5.1.Atmos.H.264-WERSJA-REŻYSERSKA-PEŁNA-NAZWA-PLIKU-GROUP.mkv\", \"behaviorHints\": {\"filename\": \"Przykładowy.Film.Z.Bardzo.Długą.Nazwą.2026.1080p.WEB-DL.DDP5.1.Atmos.H.264-WERSJA-REŻYSERSKA-PEŁNA-NAZWA-PLIKU-GROUP.mkv\"}}]}";

int main(int argc, char **argv) {
  Stream *v = NULL;
  int count;
  saida = argc > 1 ? argv[1] : "/tmp/nv-fonte-addon";
  SDL_SetHint("SDL_MAC_BACKGROUND_APP", "1");
  assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
#ifdef NV_LINUX_DESKTOP
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
  SDL_Window *w = SDL_CreateWindow("Nuvio: texto do addon", 0, 0, 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  assert(w);
  SDL_GLContext gl = SDL_GL_CreateContext(w); assert(gl); SDL_GL_SetSwapInterval(0);
  glGenTextures(1, &fboTex); glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LW, LH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glViewport(0, 0, LW, LH); gfx_tamanho_alvo(LW, LH); assert(gfx_iniciar());
  assert(txt_iniciar("deploy/app", 1)); tex_iniciar(64);
  gfx_icones_dir("deploy/app/art");
  badges_carregar("deploy/app/art");
  { char caminho[700]; FILE *f;
    snprintf(caminho, sizeof caminho, "%s/ajustes.txt", getenv("NUVIO_DADOS"));
    f = fopen(caminho, "w"); assert(f);
    fprintf(f, "idioma 0\nselected_theme 2\nvidroLocal 0\nfonteInterface %d\nfonteTextoLocal 1\n",
            TXT_FAMILIA_MONTSERRAT);
    fclose(f);
    ajustes_dir(getenv("NUVIO_DADOS")); }
  ajustes_iniciar();
  count = stream_extrair(JSON, "AIOStreams | ElfHosted", &v);
  assert(count == 3);
  assert(!strcmp(v[0].arquivo, "Przykładowy.Film.Z.Bardzo.Długą.Nazwą.2026.1080p.WEB-DL.DDP5.1.Atmos.H.264-WERSJA-REŻYSERSKA-PEŁNA-NAZWA-PLIKU-GROUP.mkv"));
  // Third source: release name is first in title/description, no filename hint.
  { char desc[sizeof v[2].descricao];
    snprintf(desc,sizeof desc,"%s\n12 GB | 18 Mbps\nEN | PL",v[2].arquivo);
    snprintf(v[2].descricao,sizeof v[2].descricao,"%s",desc); v[2].arquivo[0]=0; }
  stream_definir_alvo("tt14688458:2:5");
  stream_definir_lista(v, count);
  stream_folha_nome("Silo");
  stream_folha_contexto("T2:E5 · Silo");
  stream_folha_abrir();
  gfx_cor((GfxRect){ 0, 0, 1, 1 }, 0, 0, 0, 0, 0);
  quadros(50);
  salvar("addon-full");
  tecla(SDLK_DOWN); quadros(40); salvar("addon-next");
  { char caminho[700]; snprintf(caminho,sizeof caminho,"%s/ajustes.txt",getenv("NUVIO_DADOS"));
    FILE *f=fopen(caminho,"w"); assert(f);
    fprintf(f,"idioma 0\nselected_theme 2\nvidroLocal 0\nfonteInterface %d\nfonteTextoLocal 0\n",TXT_FAMILIA_MONTSERRAT);
    fclose(f); ajustes_dir(getenv("NUVIO_DADOS")); }
  tecla(SDLK_ESCAPE); quadros(30); stream_folha_abrir(); quadros(50);
  salvar("nuvio-full");
  tecla(SDLK_DOWN); tecla(SDLK_DOWN); quadros(40); salvar("nuvio-no-hint");
  return 0;
}
