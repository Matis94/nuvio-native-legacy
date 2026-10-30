// Rich source cards and bounded work on a 2000-source list. Local fixtures only.
#include "streams.h"
#include "nvtmp.h"
#include "limpa.h"
static int cleanCalls;
static size_t countedClean(const char *s,char *d,size_t n,int flags) {
  cleanCalls++;return nv_limpar_texto(s,d,n,flags);
}
#define nv_limpar_texto countedClean
#include "../src/streams.c"
#undef nv_limpar_texto
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <unistd.h>
#define CARDS 2000
static Stream fixtures[CARDS];
static void setting(const char *dir,int addon) {
  char path[512];snprintf(path,sizeof path,"%s/ajustes.txt",dir);
  FILE *f=fopen(path,"w");assert(f);
  fprintf(f,"fonteTextoLocal %d\nvidroLocal 0\n",addon);
  fclose(f);ajustes_dir(dir);assert(ajustes_fonte_texto_addon()==addon);
}
static int hasText(const TextoAddon *t,const char *s) {
  for(int k=0;k<t->nl;k++)for(int p=0;p<t->linhas[k].n;p++)
    if(strstr(pecaAddonTexto(&t->linhas[k],p),s))return 1;
  return 0;
}
static int hasIcon(const TextoAddon *t,const char *s) {
  for(int k=0;k<t->nl;k++)for(int p=0;p<t->linhas[k].n;p++)
    if(t->linhas[k].ic[p] && !strcmp(t->linhas[k].ic[p],s))return 1;
  return 0;
}
static int utf8Boundary(const char *s,size_t off) {
  return !s[off] || ((unsigned char)s[off]&0xc0)!=0x80;
}
static void inspectLayout(const TextoAddon *t) {
  assert(t->nv>FOLHA_ADDON_FECHADA && t->nv<=FOLHA_ADDON_VISUAIS);
  assert(t->nd<=ADDON_DESENHOS);
  for(int j=0;j<t->nd;j++) {
    const AddonDesenho *d=&t->desenhos[j];
    assert(d->linha<t->nl && d->peca<t->linhas[d->linha].n);
    assert(d->x>=0 && d->w>=0 && d->x+d->w<=t->largura+.1f);
    if(!d->icone) {
      const char *s=pecaAddonTexto(&t->linhas[d->linha],d->peca);
      assert(d->inicio+d->tam<=strlen(s));
      assert(utf8Boundary(s,d->inicio) && utf8Boundary(s,d->inicio+d->tam));
    }
  }
}
static void parserAndCache(void) {
  stream_definir_lista(fixtures,CARDS);
  int before=cleanCalls;
  const TextoAddon *t=textoAddonDa(CARDS-1);
  assert(textoAddonPreparados==1 && cleanCalls>before);
  assert(hasText(t,"DUAL") && hasText(t,"MULTI") && hasText(t,"Lektor"));
  assert(hasIcon(t,"emoji_1f1f5-1f1f1") && hasIcon(t,"emoji_1f1ec-1f1e7"));
  assert(hasIcon(t,"emoji_1f3ac"));inspectLayout(t);
  before=cleanCalls;
  for(int j=0;j<120;j++)textoAddonDa(CARDS-1);
  assert(cleanCalls==before && textoAddonPreparados==1);
  // A direct jump must not clean the preceding 1999 cards.
  assert(TEXTO_ADDON_CACHE==32 && TEXTO_ADDON_CACHE*sizeof(TextoAddon)<2*1024*1024);
  for(int k=0;k<TEXTO_ADDON_CACHE;k++)textoAddonDa(k);
  textoAddonDa(CARDS-1);
  assert(textoAddonPreparados==TEXTO_ADDON_CACHE+2);inspectLayout(textoAddonDa(CARDS-1));
  // Raw-index strides must not collide and churn the cache while scrolling.
  for(int k=0;k<8;k++)textoAddonDa(k*TEXTO_ADDON_CACHE);
  before=cleanCalls;
  for(int frame=0;frame<60;frame++)for(int k=0;k<8;k++)textoAddonDa(k*TEXTO_ADDON_CACHE);
  assert(cleanCalls==before);
  // Appending preserves existing source indices and their cached text.
  textoAddonDa(0);before=cleanCalls;
  stream_lista_acrescentar(&fixtures[0],1,0);textoAddonDa(0);
  assert(cleanCalls==before);
  textoAddonDa(CARDS);assert(cleanCalls>before);
  // A replacement of the same length invalidates every old cache entry.
  snprintf(fixtures[0].descricao,sizeof fixtures[0].descricao,"Replacement description");
  stream_definir_lista(fixtures,CARDS);t=textoAddonDa(0);
  assert(t->nl==1 && hasText(t,"Replacement description") && textoAddonPreparados==1);
  snprintf(fixtures[0].descricao,sizeof fixtures[0].descricao,"\n \n🇵🇱 DUAL MULTI Lektor\nLast line\n");
  stream_definir_lista(fixtures,CARDS);t=textoAddonDa(0);
  assert(t->nl==2 && linhasAddonContar(&lista[0])==2);
  puts("PASS: lazy parse, bounded eviction, append, replacement, emoji and UTF-8 wrapping");
}
static void geometry(const char *dir) {
  stream_definir_lista(fixtures,CARDS);setting(dir,1);
  grupo=0;linhaAnt=-1;montar(automaticaDaFolha());
  assert(nOrdem==CARDS && textoAddonPreparados==0);
  unsigned built=textoAddonPreparados;
  for(int j=0;j<CARDS;j++) {
    int first=primeiraVisivel(linhaTopo(j)+1);
    assert(first==j);
    assert(linhaAltura(j)>0);
    if(j+1<CARDS)assert(linhaTopo(j)+linhaAltura(j)<linhaTopo(j+1));
  }
  assert(textoAddonPreparados==built);
  grupo=1;foco=CARDS-1;abreFoco=1;linhaAnt=-1;
  montar(automaticaDaFolha());
  assert(textoAddonPreparados<=1);
  assert(expansaoFoco>=0 && alturaTotal>=linhaTopo(foco)+linhaAltura(foco));
  int before=cleanCalls;
  for(int j=0;j<120;j++)montar(automaticaDaFolha());
  assert(cleanCalls==before);
  soDub=1;ling_local_audio("pl");montar(automaticaDaFolha());
  assert(nOrdem==CARDS);
  before=cleanCalls;montar(automaticaDaFolha());assert(cleanCalls==before);
  soDub=0;montar(automaticaDaFolha());
  // Formatter never changes the parsed source metadata or language filter.
  assert(!strcmp(lista[CARDS-1].audioIdiomas,"pl,en"));
  setting(dir,0);montar(automaticaDaFolha());
  setting(dir,1);montar(automaticaDaFolha());
  puts("PASS: 2000-row prefixes, binary lookup, focused expansion and style switching");
}
static void emojiPixels(void) {
  assert(tex_iniciar(64));gfx_icones_dir("deploy/app/art");
  const char *path="deploy/app/art/icones/emoji_1f1f5-1f1f1.png";
  GLuint icon=0;
  for(int tries=0;tries<200 && !icon;tries++) {
    tex_bombear(8);icon=tex_obter_larg(path,48);if(!icon)SDL_Delay(5);
  }
  assert(icon);glViewport(0,0,1920,1080);
  glClearColor(77/255.f,77/255.f,77/255.f,1);glClear(GL_COLOR_BUFFER_BIT);
  assert(gfx_icone_cor((GfxRect){0,0,48,48},"emoji_1f1f5-1f1f1",1));
  unsigned char pixels[48*48*4];
  glReadPixels(0,1080-48,48,48,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
  int red=0,white=0,background=0;
  for(size_t p=0;p<sizeof pixels;p+=4) {
    red+=pixels[p]>180 && pixels[p+1]<120 && pixels[p+2]<120;
    white+=pixels[p]>180 && pixels[p+1]>180 && pixels[p+2]>180;
    background+=abs((int)pixels[p]-77)<=1 && abs((int)pixels[p+1]-77)<=1 && abs((int)pixels[p+2]-77)<=1;
  }
  assert(red>20 && white>20 && background>10);
  tex_encerrar();puts("PASS: emoji retains red/white color and transparent corners");
}
int main(void) {
  setvbuf(stdout,NULL,_IONBF,0);
  char dir[NVT_MAX];nvt_set(dir,sizeof dir,"/nuvio-rich-formatter-XXXXXX");assert(mkdtemp(dir));
  assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0);IMG_Init(IMG_INIT_PNG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
#ifdef __APPLE__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
#endif
  SDL_Window *win=SDL_CreateWindow("rich-formatter",0,0,1920,1080,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(win);
  SDL_GLContext gl=SDL_GL_CreateContext(win);assert(gl);
  gfx_tamanho_alvo(1920,1080);assert(gfx_iniciar());assert(txt_iniciar("deploy/app",1));
  for(int i=0;i<CARDS;i++) {
    Stream *s=&fixtures[i];s->altura=2160;s->mp4=s->emCache=1;s->fileIdx=-1;
    snprintf(s->url,sizeof s->url,"https://fixture.invalid/%d.mp4",i);
    snprintf(s->provedor,sizeof s->provedor,"AIOStreams");
    snprintf(s->rotulo,sizeof s->rotulo,"2160p · ⚡ Cached · TorBox %04d",i);
    snprintf(s->audioIdiomas,sizeof s->audioIdiomas,"pl,en");
    snprintf(s->descricao,sizeof s->descricao,
      "🎬 Oppenheimer (2023)\n🎞 BluRay REMUX HEVC Dolby Vision\n🎧 DTS-HD MA 5.1\n📦 89 GB · FraMeSToR\n🌐 🇵🇱 🇬🇧 DUAL MULTI Lektor\n"
      "Release.group.Oppenheimer.2023.2160p.UHD.BluRay.REMUX.HEVC.DTS-HD.MA.5.1.DUAL.MULTI.Polish.Lektor.%04d.Long.File.Name.Without.Space.mkv\n"
      "Audio: Polski · English\nSubtitles: Polish · English\nLast formatter line",i);
  }
  setting(dir,1);parserAndCache();geometry(dir);emojiPixels();
  txt_encerrar();stream_definir_lista(NULL,0);
  SDL_GL_DeleteContext(gl);SDL_DestroyWindow(win);IMG_Quit();SDL_Quit();
  char path[512];snprintf(path,sizeof path,"%s/ajustes.txt",dir);unlink(path);rmdir(dir);
  return 0;
}
