// Real list, real font metrics/GL, local data only. Exercise cold large lists,
// cache invalidation and remote navigation, not just a prewarmed 60-row list.
#include "../src/streams.c"
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <unistd.h>

static void setting(const char *dir, int addon, int order, int flags) {
  char path[1024]; snprintf(path,sizeof path,"%s/ajustes.txt",dir);
  FILE *f=fopen(path,"w"); assert(f);
  fprintf(f,"fonteTextoLocal %d\nfonteOrdemLocal %d\naudioFlagsLocal %d\nvidroLocal 1\nescolherFonteManual 1\nfonteHdrLocal 0\ndolbyVision 0\ndvMkvLocal 0\n",addon,order,!flags);
  fclose(f); ajustes_dir(dir);
  assert(ajustes_fonte_texto_addon()==addon);
  assert(ajustes_fonte_ordem_addon()==order);
  assert(ajustes_audio_flags()==flags);
}
static void sameLayout(void) {
  montar(-1);
  int total=nOrdem;
  int *indices=malloc((size_t)total*sizeof *indices);
  float *ys=malloc((size_t)total*sizeof *ys), *hs=malloc((size_t)total*sizeof *hs);
  assert(indices && ys && hs);
  memcpy(indices,ordem,(size_t)total*sizeof *indices);
  for(int i=0;i<total;i++){ys[i]=linhaPosicao(i);hs[i]=linhaAltura(i);}
  float altura=alturaTotal, secs[FOLHA_GRUPOS]; memcpy(secs,secY,sizeof secs);
  // Independent uncached layout is the correctness oracle for animation,
  // marks, dynamic row heights, group headers, filtering and addon order.
  montarLista(-1);
  assert(nOrdem==total && fabsf(altura-alturaTotal)<.15f);
  for(int i=0;i<total;i++) {
    assert(indices[i]==ordem[i]);
    if(fabsf(ys[i]-linhaY[i])>=.15f || fabsf(hs[i]-linhaH[i])>=.01f) {
      fprintf(stderr,"layout mismatch row=%d cached=(%.3f,%.3f) fresh=(%.3f,%.3f)\n",i,ys[i],hs[i],linhaY[i],linhaH[i]);
      abort();
    }
  }
  for(int g=0;g<FOLHA_GRUPOS;g++)if(secN[g])assert(fabsf(secs[g]-secY[g])<.15f);
  // Binary search must agree with the linear visibility rule at boundaries.
  for(int i=0;i<total;i+=total/13+1) {
    float top=linhaY[i]+linhaH[i]+.5f;
    int j=0;while(j<total && linhaY[j]+linhaH[j]<top)j++;
    assert(primeiraLinhaVisivel(top)==j);
  }
  free(indices);free(ys);free(hs);
}
static int numberCompare(const void *a,const void *b) { double x=*(const double *)a,y=*(const double *)b;return(x>y)-(x<y); }
static void screenshot(void) {
  const char *path=getenv("NUVIO_FLAGS_SHOT");if(!path || !*path)return;
  SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,1920,1080,32,SDL_PIXELFORMAT_RGBA32);assert(s);
  glReadPixels(0,0,1920,1080,GL_RGBA,GL_UNSIGNED_BYTE,s->pixels);
  unsigned char row[1920*4];
  for(int y=0;y<540;y++) {
    unsigned char *a=(unsigned char *)s->pixels+y*s->pitch,*b=(unsigned char *)s->pixels+(1079-y)*s->pitch;
    memcpy(row,a,sizeof row);memcpy(a,b,sizeof row);memcpy(b,row,sizeof row);
  }
  assert(IMG_SavePNG(s,path)==0);SDL_FreeSurface(s);
}
static void flagPixels(void) {
  GLuint pl=0;
  for(int j=0;j<200 && !pl;j++) {tex_bombear(16);pl=tex_obter_larg("deploy/app/art/flags/pl.png",40);if(!pl)SDL_Delay(5);}
  assert(pl);
  glClearColor(.2f,.2f,.2f,1);glClear(GL_COLOR_BUFFER_BIT);
  badges_bandeira("pl",20,20,40,30,1);
  unsigned char white[4],red[4];
  glReadPixels(40,1080-27,1,1,GL_RGBA,GL_UNSIGNED_BYTE,white);
  glReadPixels(40,1080-43,1,1,GL_RGBA,GL_UNSIGNED_BYTE,red);
  assert(white[0]>240 && white[1]>240 && white[2]>240);
  assert(red[0]>160 && red[1]<70 && red[2]<110 && "flags must retain RGB instead of badge tint");
}
static void benchmark(const char *dir, Stream *v, int count, int addon, int order) {
  setting(dir,addon,order,1);
  stream_definir_lista(v,count);
  Uint64 start=SDL_GetPerformanceCounter();
  stream_folha_abrir(); anim=1;
  double cold=(SDL_GetPerformanceCounter()-start)*1000.0/SDL_GetPerformanceFrequency();
  assert(nOrdem==count);
  if(addon)assert(textoAddonNSlots<=2 && "opening must not allocate all expanded cards");
  if(count==100 && !addon && !order && getenv("NUVIO_FLAGS_SHOT")) {
    for(int j=0;j<20;j++) {tex_bombear(32);txt_novo_quadro();glClear(GL_COLOR_BUFFER_BIT);stream_folha_desenhar(SDL_GetTicks());SDL_Delay(5);}
    screenshot();
  }
  contarFontes(); automaticoDaFolhaModo();
  unsigned builds=folhaLayoutMontagens, visits=folhaLayoutVisitas;
  unsigned counts=folhaContagemVisitas, choices=folhaAutoCalculos;
  double times[120];
  for(int j=0;j<120;j++) {
    SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_DOWN;
    Uint64 a=SDL_GetPerformanceCounter();
    txt_novo_quadro();stream_folha_evento(&e);
    stream_folha_atualizar(1.0f/60,SDL_GetTicks());stream_folha_desenhar(SDL_GetTicks());
    times[j]=(SDL_GetPerformanceCounter()-a)*1000.0/SDL_GetPerformanceFrequency();
  }
  assert(folhaLayoutMontagens==builds && "focus/scroll must not rebuild ordering and closed heights");
  assert(folhaLayoutVisitas==visits && "stable scrolling must not validate offscreen sources");
  assert(folhaContagemVisitas==counts && "drawing provider totals must not scan sources");
  assert(folhaAutoCalculos==choices && "moving focus must not recalculate automatic ranking");
  assert(textoAddonNSlots<=TEXTO_ADDON_RESIDENTES);
  qsort(times,120,sizeof *times,numberCompare);
  printf("scaling sources=%d addon=%d order=%d cold_ms=%.3f move_p50_ms=%.3f p95_ms=%.3f max_ms=%.3f full_rebuilds=%u parsed_resident=%d\n",
    count,addon,order,cold,times[60],times[114],times[119],folhaLayoutMontagens-builds,textoAddonNSlots);
  fflush(stdout);
  melhorFolha=-1; grupo=1; foco=3;linhaAnt=1;abreFoco=.37f;abreAnt=.62f;
  sameLayout();
  filtro=1;sameLayout();filtro=0;
  soMp4=1;sameLayout();soMp4=0;
  soCache=1;sameLayout();soCache=0;
  melhorFolha=5;sameLayout();
  preferida=2;atual=4;sameLayout();
  lista[6].foraCache=1;folhaDadosMudaram();sameLayout();
  fitClasses[7]=SF_PESADA;folhaDadosMudaram();sameLayout();
  setting(dir,addon,order,0);sameLayout();
  setting(dir,addon,order,1);txt_definir_fonte_interface(TXT_FAMILIA_MONTSERRAT);sameLayout();
  txt_definir_fonte_interface(TXT_FAMILIA_INTER);sameLayout();
  stream_lista_acrescentar(v,3,0);sameLayout();
  // An identical-size replacement must not reuse content, marks or order.
  stream_definir_lista(v,count);melhorFolha=-1;foco=0;linhaAnt=-1;sameLayout();
}
// Compare optimized MP4 ranking with the original pairwise policy over many
// ties, exclusions, quality buckets, class orders and negative scores.
static int mp4Reference(const Stream *v,const int *idx,long *pts,const signed char *grp,const unsigned char *excl,int total) {
  long *orig=malloc((size_t)total*sizeof *orig);assert(orig);memcpy(orig,pts,(size_t)total*sizeof *orig);
  int changed=0;
  for(int q=0;q<total;q++) {
    const Stream *s=&v[idx[q]];int g=grp[q],d=degrauRes(s),c=classeMp4(s),rank=0,tem=0;long mq=multasDeAlvo(s,NULL),teto=0;
    if(!c || excl[q] || g<0)continue;
    for(int r=0;r<total;r++) {
      const Stream *o=&v[idx[r]];
      if(r==q || excl[r] || grp[r]!=g || degrauRes(o)!=d || multasDeAlvo(o,NULL)!=mq)continue;
      int co=classeMp4(o);
      if(!co){if(!tem || orig[r]>teto)teto=orig[r];tem=1;continue;}
      if(co<c || (co==c && (orig[r]<orig[q] || (orig[r]==orig[q] && r>q))))rank++;
    }
    long novo=teto+1+rank;if(tem && novo>orig[q]){pts[q]=novo;changed++;}
  }
  free(orig);return changed;
}
static void ranking(Stream *v) {
  enum {N=64};int idx[N];long a[N],b[N];signed char grp[N];unsigned char excl[N];
  srand(487);
  for(int round=0;round<150;round++) {
    for(int j=0;j<N;j++) {
      idx[j]=(j*17+round)%N;
      v[j].altura=(int[]){0,480,720,1080,2160}[rand()%5];v[j].mp4=rand()%2;v[j].dolbyVision=rand()%2;
      v[j].badges=rand()%2?badges_bit("v-hdr10"):0;v[j].foraCache=rand()%2;
      a[j]=b[j]=rand()%12-6;grp[j]=(signed char)(rand()%4-1);excl[j]=(unsigned char)(rand()%5==0);
    }
    int ca=mp4Reference(v,idx,a,grp,excl,N),cb=mp4PrimeiroNasFontes(v,NULL,idx,b,grp,excl,N);
    assert(ca==cb && !memcmp(a,b,sizeof a));
  }
  puts("MP4 priority: 150 randomized cases match original pairwise policy");
}
int main(void) {
  char dir[]="/tmp/nuvio-list-scaling-XXXXXX";assert(mkdtemp(dir));setenv("NUVIO_DADOS",dir,1);
  assert(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==0);IMG_Init(IMG_INIT_PNG);
#ifdef NV_LINUX_DESKTOP
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
#endif
  SDL_Window *win=SDL_CreateWindow("source scaling",0,0,1920,1080,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);assert(win);
  SDL_GLContext gl=SDL_GL_CreateContext(win);assert(gl);SDL_GL_SetSwapInterval(0);
  gfx_tamanho_alvo(1920,1080);assert(gfx_iniciar());assert(txt_iniciar("deploy/app",1));
  stream_definir_tela(1,1);
  glViewport(0,0,1920,1080);assert(tex_iniciar(128));badges_carregar("deploy/app/art");gfx_icones_dir("deploy/app/art");
  flagPixels();
  Stream *v=calloc(2121,sizeof *v);assert(v);
  for(int i=0;i<2121;i++) {
    Stream *s=&v[i];s->altura=(i%3==0?2160:1080);s->mp4=i%2;s->fileIdx=-1;s->tamanhoMB=(i*7919)%60000;
    s->badges=badges_detectar("WEB-DL DTS-HD MA");
    snprintf(s->url,sizeof s->url,"https://fixture.invalid/%d.%s",i,s->mp4?"mp4":"mkv");
    snprintf(s->provedor,sizeof s->provedor,"AIOStreams %d",i%3);
    snprintf(s->rotulo,sizeof s->rotulo,"Source %d",i);
    snprintf(s->audioIdiomas,sizeof s->audioIdiomas,"Polish,English,French,German,Japanese");s->audioDescritores=i%2?STREAM_AUDIO_MULTI:STREAM_AUDIO_DUAL;
    snprintf(s->legendaIdiomas,sizeof s->legendaIdiomas,"Portuguese");
    snprintf(s->descricao,sizeof s->descricao,"🎬 Movie.2026.1080p.WEB-DL.Long.Release.Name.%04d.mkv\n🖥️ WEB-DL\n🎨 HEVC HDR DV 10Bit\n🎧 DTS-HD MA 5.1\n🗣️🌍 🇬🇧  🇵🇱 DUAL Dubbed\n💾 11.1 GB 🏷️ Group",i);
    snprintf(s->arquivo,sizeof s->arquivo,"Movie.2026.1080p.WEB-DL.Long.Release.Name.That.Must.Wrap.Without.Truncation.%04d.mkv",i);
  }
  const int counts[]={100,500,1000,2121};
  for(int c=0;c<4;c++)for(int addon=0;addon<2;addon++)for(int order=0;order<2;order++)benchmark(dir,v,counts[c],addon,order);
  setting(dir,1,1,1);stream_definir_lista(v,303);stream_folha_abrir();anim=1;
  stream_lista_acrescentar(v+303,1818,2);
  stream_folha_atualizar(1.f/60,SDL_GetTicks());
  assert(n==2121 && nOrdem==2121 && nFiltrados()==2121);
  unsigned oldChoices=folhaAutoCalculos,oldCounts=folhaContagemVisitas;
  stream_vod_falhou(4);stream_folha_atualizar(1.f/60,SDL_GetTicks());
  contarFontes();
  assert(stream_vod_falhou_ha(4)>=0 && folhaAutoCalculos>oldChoices && folhaContagemVisitas>oldCounts);
  stream_vod_tocou(4);stream_folha_atualizar(1.f/60,SDL_GetTicks());
  assert(stream_vod_falhou_ha(4)<0);sameLayout();
  ranking(v);
  stream_definir_lista(NULL,0);free(v);tex_encerrar();txt_encerrar();gfx_encerrar();SDL_GL_DeleteContext(gl);SDL_DestroyWindow(win);SDL_Quit();
  puts("source list: cold/warm scrolling, both styles/orders, layout equivalence, filters, append, replacement, fonts, flags and bounded parsed cache OK");
}
