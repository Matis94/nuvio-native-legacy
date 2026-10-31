// Rich source cards and bounded work on a 2000-source list. Local fixtures only.
#include "streams.h"
#include "nvtmp.h"
#include "limpa.h"
static int cleanCalls;
static size_t countedClean(const char *s,char *d,size_t n,int flags) {
  cleanCalls++;return nv_limpar_texto(s,d,n,flags);
}
#define NV_SHOT_HOOKS
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
  assert(t->nv>0 && t->nv<=FOLHA_ADDON_VISUAIS);
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
  assert(textoAddonPreparados==0); // Focus alone never measures hidden rows.
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
  puts("PASS: 2000-row prefixes, binary lookup, focus-independent heights and style switching");
}
static void measuredScrollGeometry(const char *dir) {
  Stream *mixed=malloc(sizeof fixtures);assert(mixed);memcpy(mixed,fixtures,sizeof fixtures);
  for(int i=0;i<CARDS;i++) {
    mixed[i].altura=i%3==0?2160:i%3==1?1080:720;
    mixed[i].tamanhoMB=(i*41%307)*1024;
    mixed[i].foraCache=i%7==0;
  }
  stream_definir_lista(mixed,CARDS);free(mixed);setting(dir,1);
  grupo=0;linhaAnt=-1;montar(automaticaDaFolha());
  unsigned builds=folhaMontagensBase;Uint64 start=SDL_GetPerformanceCounter();
  for(int batch=0;batch<60;batch++) {
    int row=(batch*31)%nOrdem;
    textoAddonDa(ordem[row]);montar(automaticaDaFolha());
  }
  double ms=(SDL_GetPerformanceCounter()-start)*1000./SDL_GetPerformanceFrequency();
  printf("formatter: 60 newly measured rows in a 2000-source list: %.2f ms, full rebuilds=%u\n",
    ms,folhaMontagensBase-builds);
  assert(folhaMontagensBase==builds);
  // Compare reflowed geometry to a fresh complete sort/filter/layout pass,
  // including quality section headings and uncached groups.
  float *ys=malloc(nOrdem*sizeof *ys),*hs=malloc(nOrdem*sizeof *hs);assert(ys && hs);
  float sections[FOLHA_GRUPOS],total=alturaTotal;
  for(int i=0;i<nOrdem;i++){ys[i]=linhaY[i];hs[i]=linhaH[i];}
  memcpy(sections,secY,sizeof sections);montarBase(automaticaDaFolha());
  assert(fabsf(alturaTotal-total)<.1f);
  for(int i=0;i<nOrdem;i++){assert(fabsf(linhaY[i]-ys[i])<.1f);assert(fabsf(linhaH[i]-hs[i])<.1f);}
  for(int g=0;g<FOLHA_GRUPOS;g++)if(secN[g])assert(fabsf(secY[g]-sections[g])<.1f);
  free(ys);free(hs);
  // Actual filter/data mutations still take the full path.
  soMp4=1;montar(automaticaDaFolha());assert(folhaMontagensBase>builds);
  soMp4=0;montar(automaticaDaFolha());
  puts("PASS: newly visited source cards reflow without repeating filters or sorting; full geometry preserved");
}
static void cardPixels(void) {
  assert(tex_iniciar(64));gfx_icones_dir("deploy/app/art");glViewport(0,0,1920,1080);
  for(int frame=0;frame<40;frame++) {
    tex_bombear(8);glClearColor(.05f,.05f,.06f,1);glClear(GL_COLOR_BUFFER_BIT);
    for(int row=0;row<2;row++) {
      GfxRect r={40,80+linhaTopo(row),FOLHA_W-FOLHA_PAD_E-FOLHA_PAD_D,linhaAltura(row)};
      desenharCartaoAddon(ordem[row],row,r,r.x+FOLHA_TXT,r.x+r.w-FOLHA_TXT,
        automaticaDaFolha(),row==foco,1,SDL_GetTicks());
    }
    SDL_Delay(5);
  }
  unsigned char *pixels=malloc(820*1080*4);assert(pixels);glFinish();
  glReadPixels(0,0,820,1080,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
  for(int row=0;row<2;row++) {
    const TextoAddon *t=textoAddonDa(ordem[row]);
    float body=80+linhaTopo(row)+FOLHA_ADDON_TOPO+FOLHA_ADDON_SELOS+FOLHA_ADDON_TITULO+
      (temMarca(ordem[row],automaticaDaFolha())?FOLHA_ADDON_MARCA:0);
    // The last metadata row must actually render on both cards,
    // including the card without focus (the original screenshot bug).
    int y0=(int)(body+(t->nv-1)*FOLHA_ADDON_LD),ink=0;
    for(int y=y0;y<y0+(int)FOLHA_ADDON_LD;y++)for(int x=66;x<730;x++) {
      unsigned char *p=pixels+((1079-y)*820+x)*4;
      if(p[0]>90 && p[1]>90 && p[2]>90)ink++;
    }
    assert(ink>60);
  }
  const char *shot=getenv("NUVIO_FORMATTER_SHOT");
  if(shot) {
    SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,820,1080,32,SDL_PIXELFORMAT_RGBA32);assert(s);
    for(int y=0;y<1080;y++)memcpy((char *)s->pixels+y*s->pitch,pixels+(1079-y)*820*4,820*4);
    assert(IMG_SavePNG(s,shot)==0);SDL_FreeSurface(s);
  }
  free(pixels);tex_encerrar();puts("PASS: final formatter metadata renders on selected and unselected cards");
}
static void completeCards(const char *dir,int pixels) {
  Stream cards[4]={0};
  for(int i=0;i<3;i++) {
    cards[i].altura=1080;cards[i].emCache=1;cards[i].fileIdx=-1;
    snprintf(cards[i].url,sizeof cards[i].url,"https://fixture.invalid/%d.mkv",i);
    snprintf(cards[i].provedor,sizeof cards[i].provedor,"AIOStreams");
    snprintf(cards[i].rotulo,sizeof cards[i].rotulo,"🇵🇱 [TB ⚡] %s 1080p",i==1?"MF|Midnight":"ST Store");
    snprintf(cards[i].arquivo,sizeof cards[i].arquivo,"Chicago.Fire.S09E11.A.Couple.Hundred.Degrees.1080p.BluRay.REMUX.AVC.DTS-HD.MA.5.1-NOGRP.mkv");
    snprintf(cards[i].descricao,sizeof cards[i].descricao,
      "🎬 %s\n📺 WEB-DL\n🎨 HEVC\n🎧 DTS-HD MA 5.1\n🗣 🇵🇱 🇬🇧 🇩🇪\n💾 9.6 GB ⚙ RARBG",cards[i].arquivo);
  }
  snprintf(cards[0].rotulo,sizeof cards[0].rotulo,"[TB+] Torrentio 1080p");
  snprintf(cards[0].descricao,sizeof cards[0].descricao,
    "Chicago.Fire.S09.1080p.BluRay.REMUX.AVC.DTS-HD.MA.5.1-NOGRP[artv]\n%s\n"
    "👤 2 💾 9.6 GB ⚙ RARBG\n🎧 DTS-HD MA 5.1 🇵🇱 🇬🇧",cards[0].arquivo);
  stream_definir_lista(cards,3);setting(dir,1);folhaRelogio=1;folhaGeometria();
  grupo=1;foco=0;abreFoco=1;linhaAnt=-1;rolagem=0;
  montar(automaticaDaFolha());prepararCartoesAddon(automaticaDaFolha());
  for(int i=0;i<3;i++)textoAddonDa(i);
  montar(automaticaDaFolha());
  assert(textoAddonPreparados==3);
  float heights[3],tops[3];
  for(int row=0;row<3;row++) {
    const TextoAddon *t=textoAddonDa(ordem[row]);
    assert(hasText(t,"DTS-HD MA 5.1") && hasText(t,"9.6 GB"));
    assert(hasIcon(t,"emoji_1f1f5-1f1f1") && hasText(t,"NOGRP.mkv"));
    // Restore each addon newline instead of joining codec/audio/size together.
    int last=-1;
    for(int j=0;j<t->nd;j++) {
      const AddonDesenho *d=&t->desenhos[j];
      if(j && d->linha!=t->desenhos[j-1].linha)assert(d->visual>last);
      last=d->visual;
    }
    assert(t->nv>=linhasAddonContar(&lista[ordem[row]]));
    heights[row]=linhaAltura(row);tops[row]=linhaTopo(row);
    float body=FOLHA_ADDON_TOPO+FOLHA_ADDON_SELOS+FOLHA_ADDON_TITULO+
      (temMarca(ordem[row],automaticaDaFolha())?FOLHA_ADDON_MARCA:0);
    assert(body+t->nv*FOLHA_ADDON_LD+FOLHA_ADDON_BASE==heights[row]);
  }
  int cleaned=cleanCalls;
  for(int frame=0;frame<120;frame++) {
    linhaAnt=foco;foco=(foco+1)%3;abreFoco=(frame%10)/10.f;abreAnt=1-abreFoco;
    montar(automaticaDaFolha());prepararCartoesAddon(automaticaDaFolha());
    for(int row=0;row<3;row++)assert(linhaAltura(row)==heights[row] && linhaTopo(row)==tops[row]);
  }
  assert(cleanCalls==cleaned);
  if(pixels)cardPixels();
  cards[3]=cards[2];snprintf(cards[3].url,sizeof cards[3].url,"https://fixture.invalid/3.mkv");
  stream_definir_lista(cards,4);rolagem=0;grupo=1;foco=0;linhaAnt=-1;
  montar(automaticaDaFolha());prepararCartoesAddon(automaticaDaFolha());
  // The original centered scrolling must expose the whole selected card.
  for(foco=0;foco<4;foco++) {
    textoAddonDa(ordem[foco]);montar(automaticaDaFolha());
    rolagem=alvoRolagem();prepararCartoesAddon(automaticaDaFolha());
    rolagem=alvoRolagem();
    assert(linhaTopo(foco)>=rolagem);
    assert(linhaTopo(foco)+linhaAltura(foco)<=rolagem+areaLista());
  }
  // A measured height survives replacement of all rich LRU entries.
  stream_definir_lista(fixtures,CARDS);montar(automaticaDaFolha());
  const TextoAddon *t=textoAddonDa(0);int rows=t->nv;
  for(int i=1;i<=TEXTO_ADDON_CACHE;i++)textoAddonDa(i);
  assert(addonLinhasVisuais(0)==rows);
  puts("PASS: original row spacing and scrolling, audio/flags/size/filename retained, no collapse or parsing on focus");
}
// Separate AIOStreams rows from the reported 4K/DV regression screenshot.
static void screenshotCards(const char *dir,int pixels) {
  Stream cards[3]={0};
  const char *names[]={
    "The.Godfather.Part.II.1974.2160p.UHD.BDRemux.TrueHD.5.1.DoVi.P8.by.DVT.mkv",
    "The.Godfather.Part.II.1974.2160p.UHD.BDRemux.TrueHD.5.1.DoVi.P7.mkv",
    "The.Godfather.Part.II.1974.2160p.UltraHD.BluRay.REMUX.x265.TRUEHD.MULTI.TPB-RR-GuyZo.mkv"};
  for(int i=0;i<3;i++) {
    cards[i].altura=2160;cards[i].dolbyVision=1;cards[i].emCache=1;cards[i].fileIdx=-1;
    snprintf(cards[i].url,sizeof cards[i].url,"https://fixture.invalid/godfather-%d.mkv",i);
    snprintf(cards[i].provedor,sizeof cards[i].provedor,"AIOStreams|PL");
    snprintf(cards[i].rotulo,sizeof cards[i].rotulo,"☁ [TB ⚡] ST Store 4K");
    snprintf(cards[i].arquivo,sizeof cards[i].arquivo,"%s",names[i]);
    snprintf(cards[i].descricao,sizeof cards[i].descricao,
      "🎬 %s\n📀 REMUX\n🎨 HEVC HDR DV\n🎧 DD DTS-HD MA TrueHD 5.1 2.0\n🗣 🇵🇱 🇬🇧\n💾 113.35 GB",names[i]);
    cards[i].badges=badges_detectar(cards[i].descricao);
  }
  stream_definir_lista(cards,3);setting(dir,1);rolagem=0;grupo=1;foco=0;linhaAnt=-1;
  montar(automaticaDaFolha());prepararCartoesAddon(automaticaDaFolha());
  for(int i=0;i<3;i++) {
    const TextoAddon *t=textoAddonDa(i);inspectLayout(t);
    assert(t->nl==6 && hasText(t,"113.35 GB") && hasText(t,"DD DTS-HD MA TrueHD 5.1 2.0"));
    // Every byte of the wrapped filename must survive, and each metadata line
    // must start below the previous one, selected or unselected.
    size_t bytes=0;int previous=-1,line=-1;
    for(int j=0;j<t->nd;j++) {
      const AddonDesenho *d=&t->desenhos[j];
      if(d->linha!=line)assert((int)d->visual>previous);
      line=d->linha;previous=d->visual;
      if(d->linha==0 && !d->icone)bytes+=d->tam;
    }
    assert(bytes==strlen(pecaAddonTexto(&t->linhas[0],0)));
    assert(t->nv>=t->nl);
  }
  montar(automaticaDaFolha());
  if(pixels)cardPixels();
  puts("PASS: Godfather DV screenshot fixture keeps separate audio/size rows and complete filenames");
}
static void firstCardScrollPixels(const char *dir) {
  setting(dir,1);folhaRelogio=0;folhaGeometria();
  grupo=1;foco=1;focoVisto=1;linhaAnt=-1;abreFoco=1;abreAnt=0;
  aberta=1;anim=1;focoFixo=-1;
  montar(automaticaDaFolha());
  for(int i=0;i<nOrdem;i++)textoAddonDa(ordem[i]);
  montar(automaticaDaFolha());rolagem=alvoRolagem();velRol=0;
  assert(rolagem>8 && linhaTopo(0)==0);
  foco=0;
  assert(tex_iniciar(64));gfx_icones_dir("deploy/app/art");
  float previousClip=0,previousScroll=rolagem;
  int crossed=0,visible=0;
  unsigned char pixels[600*8*4];
  for(int frame=0;frame<120;frame++) {
    txt_novo_quadro();tex_novo_quadro();tex_bombear(8);
    stream_folha_atualizar(1.f/60,SDL_GetTicks());
    glClearColor(.05f,.05f,.06f,1);glClear(GL_COLOR_BUFFER_BIT);
    corpoFolha(0,FOLHA_W,1,SDL_GetTicks(),0);
    glFinish();
    float clip=stream_folha_recorte_y();
    // Returning through the old one-pixel threshold must not jump the clip
    // eight pixels while the card itself moves only a fraction of a pixel.
    if(frame)assert(fabsf(clip-previousClip)<=fabsf(rolagem-previousScroll)+.01f);
    if(previousScroll>1 && rolagem<=1)crossed=1;
    previousClip=clip;previousScroll=rolagem;
    glReadPixels(80,1080-(int)FOLHA_TOPO-4,600,8,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    int ink=0;
    for(size_t k=0;k<sizeof pixels;k+=4)
      if(pixels[k]>130 && pixels[k+1]>130 && pixels[k+2]>130)ink++;
    if(rolagem<1) {
      assert(ink>200);visible++;
    }
    if(rolagem>8)assert(clip==FOLHA_TOPO);
    // The list must never paint over the addon tabs.
    assert(clip>=FOLHA_ABAS_Y+folhaOY+FOLHA_ABAS_H);
  }
  assert(crossed && visible>30);
  tex_encerrar();aberta=0;anim=0;
  puts("PASS: first-card return animation keeps its top border and a continuous clip below addon tabs");
}
static void wrappedFilenamePixels(void) {
  const char *prefix="Chicago.Fire.S09E11.A.Couple.Hundred.Degrees.";
  const char *filename="Chicago.Fire.S09E11.A.Couple.Hundred.Degrees.1080p.BluRay.REMUX.AVC.DTS-HD.MA.5.1-NOGRP.mkv";
  LinhaAddon line;pecasAddon(filename,strlen(filename),&line);
  AddonDesenho draw[ADDON_DESENHOS];int count=0,rows=0;
  float width=(float)txt_largura(TXT_PG_FIM,prefix);
  addonQuebrar(&line,1,draw,ADDON_DESENHOS,&count,&rows,width,TXT_PG_FIM,FOLHA_ADDON_VISUAIS);
  assert(rows>1 && draw[0].tam==strlen(prefix) && draw[0].w==width);
  size_t bytes=0;for(int j=0;j<count;j++)if(!draw[j].icone)bytes+=draw[j].tam;
  assert(bytes==strlen(filename));
  // Subtracting animated panel coordinates can leave the drawing width one
  // float step below its cached layout width. Already wrapped text must stay
  // byte-for-byte and pixel-for-pixel identical rather than acquire ellipses.
  unsigned char *pixels[2]={malloc(800*400*4),malloc(800*400*4)};
  assert(pixels[0] && pixels[1]);
  for(int frame=0;frame<20;frame++)for(int pass=0;pass<2;pass++) {
    txt_novo_quadro();glClearColor(.05f,.05f,.06f,1);glClear(GL_COLOR_BUFFER_BIT);
    addonDesenharPecas(&line,draw,count,40,80,pass?nextafterf(width,0):width,
      rows*FOLHA_ADDON_LD,TXT_PG_FIM,1,1);
    glFinish();
    if(frame==19)glReadPixels(0,1080-400,800,400,GL_RGBA,GL_UNSIGNED_BYTE,pixels[pass]);
  }
  assert(!memcmp(pixels[0],pixels[1],800*400*4));
  free(pixels[0]);free(pixels[1]);
  puts("PASS: wrapped full filenames never acquire ellipses from fractional panel widths");
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
int main(int argc,char **argv) {
  int logicOnly=argc>1 && !strcmp(argv[1],"--logic-only");
  setvbuf(stdout,NULL,_IONBF,0);
  char dir[NVT_MAX];nvt_set(dir,sizeof dir,"/nuvio-rich-formatter-XXXXXX");assert(mkdtemp(dir));
  assert(SDL_Init(SDL_INIT_TIMER|(logicOnly?0:SDL_INIT_VIDEO))==0);IMG_Init(IMG_INIT_PNG);
  SDL_Window *win=NULL;SDL_GLContext gl=NULL;
  if(!logicOnly) {
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
#ifdef __APPLE__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
#endif
  win=SDL_CreateWindow("rich-formatter",0,0,1920,1080,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(win);
  gl=SDL_GL_CreateContext(win);assert(gl);
  gfx_tamanho_alvo(1920,1080);assert(gfx_iniciar());
  }
  assert(txt_iniciar("deploy/app",1));
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
  const char *symbols[]={"💿","🧲","📂","♻️","🇦🇷","👩‍💻","1️⃣","👨🏽‍💻"};
  for(unsigned k=0;k<sizeof symbols/sizeof *symbols;k++) {
    size_t used=0;assert(emojiIcone(symbols[k],&used));assert(used==strlen(symbols[k]));
  }
  size_t ignored=0;assert(!emojiIcone("1080p.filename.mkv",&ignored));
  // Every supported emoji uses the longest match, including ZWJ, skin tones,
  // keycaps and optional VS16. This catches prefix matches eating only a base.
  for(unsigned k=0;k<sizeof EMOJI_ICONE/sizeof *EMOJI_ICONE;k++) {
    size_t used=0;const char *icon=emojiIcone(EMOJI_ICONE[k].emoji,&used);
    assert(icon && !strcmp(icon,EMOJI_ICONE[k].icone));
    assert(used==strlen(EMOJI_ICONE[k].emoji));
  }
  const char *longname="Show.S01E01.2160p.UHD.BluRay.REMUX.HEVC.Dolby.Vision.TrueHD.Atmos.7.1.Polish.Lektor.DUAL.MULTI.Extended.Directors.Cut.Release.Group.FraMeSToR.Full.Filename.End.mkv";
  const ArquivoLayout *arq=arquivoLayout(longname);size_t end=0;
  for(int k=0;k<arq->n;k++){assert(arq->off[k]==end);end+=arq->tam[k];}
  assert(end==strlen(longname) && arq->n>1);
  assert(arquivoAltura(longname)==arq->n*FOLHA_ARQ_H);
  setting(dir,1);parserAndCache();geometry(dir);measuredScrollGeometry(dir);completeCards(dir,!logicOnly);screenshotCards(dir,!logicOnly);if(!logicOnly){firstCardScrollPixels(dir);wrappedFilenamePixels();emojiPixels();}
  txt_encerrar();stream_definir_lista(NULL,0);
  if(gl)SDL_GL_DeleteContext(gl);if(win)SDL_DestroyWindow(win);IMG_Quit();SDL_Quit();
  char path[512];snprintf(path,sizeof path,"%s/ajustes.txt",dir);unlink(path);rmdir(dir);
  return 0;
}
