// Presentation only: source ranking, filters, track selection and playback
// continue to use the same Stream. Called only for visible From addon cards.
// Text already snaps to the drawable pixel grid in text.c. Give the whole
// formatter card that same moving origin, so unsnapped emoji/artwork cannot
// slide by a fraction of a pixel relative to independently snapped text.
// This affects drawing only; scrolling, row heights and hit testing stay float.
static float addonPixel(float value, float pixels) {
  return pixels > 0 ? roundf(value * pixels) / pixels : value;
}
static void addonAlinharCartao(GfxRect *r, float *left, float *right) {
  float pixels = gfx_px_por_unidade() * gfx_escala();
  if (pixels <= 0) return;
  float leftOffset = addonPixel(*left - r->x, pixels), width = *right - *left;
  r->x = addonPixel(r->x, pixels);
  r->y = addonPixel(r->y, pixels);
  *left = r->x + leftOffset; *right = *left + width;
}
static void addonDesenharPecas(const LinhaAddon *ls,const AddonDesenho *ds,int nd,
                               float x,float y,float largura,
                               float altura,TxtEstilo estilo,int sel,float anim,
                               float clipTop,float clipBottom) {
  float pixels=gfx_px_por_unidade()*gfx_escala();
  x=addonPixel(x,pixels);y=addonPixel(y,pixels);
  for(int j=0;j<nd;j++) {
    const AddonDesenho *d=&ds[j];
    float dx=addonPixel(d->x,pixels),dy=addonPixel(d->visual*FOLHA_ADDON_LD,pixels);
    if(dy+22>altura+.5f)break;
    // The card can be much taller than the viewport (long filenames). The
    // GL scissor hid these pixels already; avoid submitting invisible pieces.
    // Keep rounding margin even on a scaled interface.
    if(y+dy>clipBottom+4.0f)break;
    float a=anim;
    if(a<.01f)continue;
    const LinhaAddon *l=&ls[d->linha];
    int c=sel ? (d->linha?205:230) : (d->linha?165:195);
    if(estilo==TXT_CALLOUT)c=sel?250:225;
    if(d->icone) {
      GfxRect icon={x+dx,y+dy+addonPixel(3,pixels),22,22};
      if(icon.y+icon.h<clipTop-4.0f)continue;
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
      if(y+dy+line.h<clipTop-4.0f)continue;
      txt_desenhar_tom(line,x+dx,y+dy,c/255.f,c/255.f,c/255.f,a);
    }
  }
}
static GfxRect addonFocoRect(GfxRect r,int row) {
  if(row==0 && r.y<FOLHA_TOPO) {
    r.h-=FOLHA_TOPO-r.y;r.y=FOLHA_TOPO;
  }
  return r;
}
// Reorder the same three blocks. Moving badges changes no card height and
// needs no new text measurement or source-list rebuild while scrolling.
static void addonPosicoes(const TextoAddon *t, const Stream *s, float base, int bottom,
                           float *title, float *body, float *badges) {
  float bodyHeight = t->nv * FOLHA_ADDON_LD;
  if (!t->nl) bodyHeight += linhasFaixas(s) * FOLHA_FAIXA_H;
  *title = base + (bottom ? 0 : FOLHA_ADDON_SELOS);
  *body = *title + FOLHA_ADDON_TITULO;
  *badges = bottom ? *body + bodyHeight + (FOLHA_ADDON_SELOS - FOLHA_SELO_H) * .5f : base;
}
static void desenharCartaoAddon(int i,int row,GfxRect r,float tx,float tr,
                                int automatica,int sel,float anim,Uint32 agora,
                                float clipTop,float clipBottom) {
  const Stream *s=&lista[i];const TextoAddon *t=textoAddonDa(i);
  float largura=tr-tx;
  addonAlinharCartao(&r,&tx,&tr);
  float pixels=gfx_px_por_unidade()*gfx_escala();
  // Calculate block offsets before adding the animated card origin. This
  // keeps half-pixel rounding consistent at reduced/enlarged UI scales too.
  float y=FOLHA_ADDON_TOPO;
  float bodyX=tx,bodyW=largura;
  if(sel) {
    GfxRect ring=addonFocoRect(r,row);
    if(ring.h>4)gfx_anel(ring,fminf(FOLHA_RAIO,ring.h*.5f)/ring.h,2,.72f,.72f,.76f,.9f*anim);
  }
  if(temMarca(i,automatica)) {
    const char *marca=i==atual ? "Reproduzindo agora" : i==preferida ? "Sua escolha anterior" :
      fitPesada(i) ? "Acima da conexão" : s->foraCache && !cacheConfirmado(s) ? "Fora do cache" :
      (!canalFolha && stream_vod_falhou_ha(i)>=0) ? "Falhou" : "Escolha automática";
    txt_desenhar_alpha(txt_linha_corta(TXT_PG_FIM,marca,185,185,183,255,largura-32),tx,r.y+addonPixel(y,pixels),anim);
    if(i==atual)desenharAudioBars(tr-60,r.y+addonPixel(y+3,pixels),14,anim,agora);
    y+=FOLHA_ADDON_MARCA;
  }
  float titleY, bodyY, badgesY;
  addonPosicoes(t,s,y,ajustes_selos_embaixo(),&titleY,&bodyY,&badgesY);
  y=r.y+addonPixel(badgesY,pixels);
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
  int badgeView=selosVistaDa((Stream *)s,0);
  if(badgeView==SELOS_VISTA_PACOTE)
    desenharSelosPacote(s,tx,y,bw,FOLHA_SELO_H,anim*(sel || selospacote_ativo()>=0?1:.85f),sel?1:.85f,0);
  else if(badgeView==SELOS_VISTA_LEGADO) {
    uint64_t mask=logosDa(s,0) | (s->badges & (badges_bit("r-4k")|badges_bit("r-1080")|badges_bit("r-720")|badges_bit("r-sd")));
    float used=ajustes_selos_coloridos() ? badges_desenhar_selos(mask,tx,y,bw,FOLHA_SELO_H,anim) :
      badges_desenhar_tom(mask,tx,y,bw,FOLHA_SELO_H,.85f,.85f,.85f,anim);
    if(!s->dolbyVision && badges_fonte_hdr_marca(s->badges)<0 && used+64<bw)
      marca_formato(FMT_SDR,tx+used+(used?12:0),y,FOLHA_SELO_H,.7f,.7f,.7f,anim);
  }
  y=r.y+addonPixel(titleY,pixels);
  addonDesenharPecas(&t->cabecalho,t->nome,t->nn,bodyX,y,bodyW,36,TXT_CALLOUT,sel,anim,clipTop,clipBottom);
  y=r.y+addonPixel(bodyY,pixels);
  addonDesenharPecas(t->linhas,t->desenhos,t->nd,bodyX,y,bodyW,
                     t->nv*FOLHA_ADDON_LD,TXT_PG_FIM,sel,anim,clipTop,clipBottom);
  if(!t->nl)desenharFaixas(s,bodyX,y,bodyW,sel,anim);
}
