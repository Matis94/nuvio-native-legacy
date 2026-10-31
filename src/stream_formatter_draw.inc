// Presentation only: source ranking, filters, track selection and playback
// continue to use the same Stream. Called only for visible From addon cards.
static void addonDesenharPecas(const LinhaAddon *ls,const AddonDesenho *ds,int nd,
                               float x,float y,float largura,
                               float altura,TxtEstilo estilo,int sel,float anim) {
  for(int j=0;j<nd;j++) {
    const AddonDesenho *d=&ds[j];
    float dy=d->visual*FOLHA_ADDON_LD;
    if(dy+22>altura+.5f)break;
    float a=anim;
    if(a<.01f)continue;
    const LinhaAddon *l=&ls[d->linha];
    int c=sel ? (d->linha?205:230) : (d->linha?165:195);
    if(estilo==TXT_CALLOUT)c=sel?250:225;
    if(d->icone) {
      GfxRect icon={x+d->x,y+dy+3,22,22};
      if(!gfx_icone_cor(icon,l->ic[d->peca],a))
        gfx_icone(icon,emojiReserva(l->ic[d->peca]),c/255.f,c/255.f,c/255.f,a);
    } else {
      char texto[2048];size_t tam=d->tam;
      if(tam>=sizeof texto)tam=sizeof texto-1;
      memcpy(texto,pecaAddonTexto(l,d->peca)+d->inicio,tam);texto[tam]=0;
      // Body fragments are already wrapped by addonQuebrar. Do not shorten
      // them again when animated panel arithmetic rounds the width down.
      // The single-row addon heading still uses its existing ellipsis policy.
      TxtLinha line=estilo==TXT_CALLOUT
        ? txt_linha_corta(estilo,texto,255,255,255,255,fmaxf(1,largura-d->x))
        : txt_linha(estilo,texto,255,255,255,255);
      txt_desenhar_tom(line,x+d->x,y+dy,c/255.f,c/255.f,c/255.f,a);
    }
  }
}
static GfxRect addonFocoRect(GfxRect r,int row) {
  if(row==0 && r.y<FOLHA_TOPO) {
    r.h-=FOLHA_TOPO-r.y;r.y=FOLHA_TOPO;
  }
  return r;
}
static void desenharCartaoAddon(int i,int row,GfxRect r,float tx,float tr,
                                int automatica,int sel,float anim,Uint32 agora) {
  const Stream *s=&lista[i];const TextoAddon *t=textoAddonDa(i);
  float y=r.y+FOLHA_ADDON_TOPO,largura=tr-tx;
  float bodyX=tx,bodyW=largura;
  if(sel) {
    GfxRect ring=addonFocoRect(r,row);
    if(ring.h>4)gfx_anel(ring,fminf(FOLHA_RAIO,ring.h*.5f)/ring.h,2,.72f,.72f,.76f,.9f*anim);
  }
  if(temMarca(i,automatica)) {
    const char *marca=i==atual ? "Reproduzindo agora" : i==preferida ? "Sua escolha anterior" :
      fitPesada(i) ? "Acima da conexão" : s->foraCache && !cacheConfirmado(s) ? "Fora do cache" :
      (!canalFolha && stream_vod_falhou_ha(i)>=0) ? "Falhou" : "Escolha automática";
    txt_desenhar_alpha(txt_linha_corta(TXT_PG_FIM,marca,185,185,183,255,largura-32),tx,y,anim);
    if(i==atual)desenharAudioBars(tr-60,y+3,14,anim,agora);
    y+=FOLHA_ADDON_MARCA;
  }
  // Badges live in their own full-width row and do not compete with filename,
  // language flags or the addon identity for horizontal space.
  float tamanhoW=0;
  if(!s->descricao[0] && s->tamanhoMB>0) {
    char gb[40];snprintf(gb,sizeof gb,"%.1f GB",s->tamanhoMB/1024.0);plrui_decimal(gb);
    tamanhoW=badge_largura(gb);
    if(tamanhoW<largura*.4f)badge_desenhar(tr-tamanhoW,y,gb,BADGE_NEUTRO,anim);
    else tamanhoW=0;
  }
  float bw=fmaxf(40,largura-(tamanhoW>0 ? tamanhoW+12 : 0));
  if(selosPacoteDa((Stream *)s)>0)
    desenharSelosPacote(s,tx,y,bw,FOLHA_SELO_H,anim*(sel?1:.85f),sel?1:.85f,0);
  else {
    uint64_t mask=logosDa(s,0) | (s->badges & (badges_bit("r-4k")|badges_bit("r-1080")|badges_bit("r-720")|badges_bit("r-sd")));
    float used=ajustes_selos_coloridos() ? badges_desenhar_selos(mask,tx,y,bw,FOLHA_SELO_H,anim) :
      badges_desenhar_tom(mask,tx,y,bw,FOLHA_SELO_H,.85f,.85f,.85f,anim);
    if(!s->dolbyVision && badges_fonte_hdr_marca(s->badges)<0 && used+64<bw)
      marca_formato(FMT_SDR,tx+used+(used?12:0),y,FOLHA_SELO_H,.7f,.7f,.7f,anim);
  }
  y+=FOLHA_ADDON_SELOS;
  addonDesenharPecas(&t->cabecalho,t->nome,t->nn,bodyX,y,bodyW,36,TXT_CALLOUT,sel,anim);
  y+=FOLHA_ADDON_TITULO;
  addonDesenharPecas(t->linhas,t->desenhos,t->nd,bodyX,y,bodyW,
                     t->nv*FOLHA_ADDON_LD,TXT_PG_FIM,sel,anim);
  if(!t->nl)desenharFaixas(s,bodyX,y,bodyW,sel,anim);
}
