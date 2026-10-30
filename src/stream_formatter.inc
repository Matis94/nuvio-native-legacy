// Rich addon cards. Parsing and text measurement are lazy, bounded and kept
// outside scroll animation. The list prefix only needs cheap newline counts.
#define FOLHA_ADDON_LINHAS 12
#define FOLHA_ADDON_FECHADA 5
#define FOLHA_ADDON_VISUAIS 16
#define FOLHA_ADDON_LD 28.0f
#define ADDON_PECAS 24
#define ADDON_DESENHOS 192
#define TEXTO_ADDON_CACHE 32
#include "stream_formatter_emoji.inc"

typedef struct {
  int n;
  const char *ic[ADDON_PECAS];
  unsigned short off[ADDON_PECAS];
  char texto[2048];
} LinhaAddon;
typedef struct {
  unsigned short inicio, tam;
  unsigned char linha, peca, visual, icone;
  float x, w;
} AddonDesenho;
typedef struct {
  char titulo[192];
  LinhaAddon cabecalho, linhas[FOLHA_ADDON_LINHAS];
  AddonDesenho desenhos[ADDON_DESENHOS], nome[ADDON_PECAS * 2];
  int nl, nv, nd, nn, indice, pronto;
  int corpo, peso, familia;
  unsigned uso;
  float largura;
} TextoAddon;
static TextoAddon *textoAddon;
static unsigned textoAddonPreparados; // regression instrumentation, no user data
static unsigned textoAddonRelogio;

static void linhaLimpa(char *d, size_t tam, const char *ini, size_t n) {
  size_t k = 0;
  while (n && (*ini == ' ' || *ini == '\t')) { ini++; n--; }
  while (n && (ini[n-1] == ' ' || ini[n-1] == '\t' || ini[n-1] == '\r')) n--;
  while (k < n && k + 1 < tam) { unsigned char c = (unsigned char)ini[k]; d[k++] = c < 32 ? ' ' : (char)c; }
  d[k] = 0;
}
static const char *emojiIcone(const char *p, size_t *len) {
  // ASCII filenames dominate large result lists. None of our emoji start
  // with ASCII or a UTF-8 continuation byte; do not scan 83 assets per byte.
  unsigned char primeiro=(unsigned char)*p;
  if(primeiro!=0xF0 && primeiro!=0xE2)return NULL;
  for (size_t k = 0; k < sizeof EMOJI_ICONE / sizeof *EMOJI_ICONE; k++) {
    size_t n = strlen(EMOJI_ICONE[k].emoji);
    if (strncmp(p, EMOJI_ICONE[k].emoji, n)) continue;
    if (!strncmp(p+n, "\xEF\xB8\x8F", 3)) n += 3;
    *len = n; return EMOJI_ICONE[k].icone;
  }
  return NULL;
}
static const char *emojiReserva(const char *icone) {
  for (size_t k = 0; k < sizeof EMOJI_ICONE / sizeof *EMOJI_ICONE; k++)
    if (!strcmp(EMOJI_ICONE[k].icone,icone)) return EMOJI_ICONE[k].reserva;
  return "aj_globe";
}
static const char *pecaAddonTexto(const LinhaAddon *l, int p) { return l->texto + l->off[p]; }
static void pecasAddon(const char *ini, size_t n, LinhaAddon *l) {
  char cru[2048]; size_t usado = 0;
  memset(l, 0, sizeof *l);
  linhaLimpa(cru, sizeof cru, ini, n);
  for (const char *p = cru; *p && l->n < ADDON_PECAS && usado + 1 < sizeof l->texto;) {
    size_t el = 0; const char *icone = emojiIcone(p, &el), *q = p + el;
    const char *fim = q;
    while (*fim) { size_t z; if (emojiIcone(fim, &z)) break; fim++; }
    char seg[2048]; size_t tamanho = (size_t)(fim-q);
    if (tamanho >= sizeof seg) tamanho = sizeof seg-1;
    memcpy(seg,q,tamanho); seg[tamanho]=0;
    l->off[l->n] = (unsigned short)usado;
    nv_limpar_texto(seg,l->texto+usado,sizeof l->texto-usado,NV_LIMPA_UMA_LINHA);
    nv_aparar_separadores(l->texto+usado);
    if (icone || l->texto[usado]) {
      l->ic[l->n++] = icone;
      usado += strlen(l->texto+usado)+1;
    }
    p = fim;
  }
}
static int linhasAddon(const Stream *s, LinhaAddon *out, int max) {
  int nl = 0;
  for (const char *p = s->descricao; *p && nl < max;) {
    const char *f = strchr(p,'\n');
    pecasAddon(p,f ? (size_t)(f-p) : strlen(p),&out[nl]);
    if (out[nl].n) nl++;
    if (!f) break;
    p=f+1;
  }
  if(max>0 && s->arquivo[0] && !strstr(s->descricao,s->arquivo)) {
    if(nl==max)nl--;
    pecasAddon(s->arquivo,strlen(s->arquivo),&out[nl]);
    if(out[nl].n)nl++;
  }
  return nl;
}
// Used for offscreen geometry: no normalization, glyph lookup, font measure,
// texture creation, or prefix parsing up to a distant source index.
static int linhasAddonContar(const Stream *s) {
  int nl=0, tem=0;
  for (const unsigned char *p=(const unsigned char *)s->descricao;;p++) {
    if (!*p || *p=='\n') {
      if (tem && ++nl == FOLHA_ADDON_LINHAS) break;
      tem=0; if(!*p)break;
    } else if (*p>' ') tem=1;
  }
  if(nl<FOLHA_ADDON_LINHAS && s->arquivo[0] && !strstr(s->descricao,s->arquivo))nl++;
  return nl;
}
static void tituloAddon(const Stream *s, char *buf, size_t tam) {
  nv_limpar_texto(s->rotulo,buf,tam,NV_LIMPA_UMA_LINHA);
  if (!buf[0]) snprintf(buf,tam,"%s",s->provedor);
}
static float addonTextoW(void) {
  float w=FOLHA_W-FOLHA_PAD_E-FOLHA_PAD_D-2*FOLHA_TXT;
  return fmaxf(80,w);
}
static int addonMedir(const char *s, size_t n, TxtEstilo estilo) {
  char buf[2048]; if(n>=sizeof buf)n=sizeof buf-1;
  memcpy(buf,s,n);buf[n]=0;return txt_largura(estilo,buf);
}
// Long filenames wrap at punctuation or a UTF-8 boundary, never disappear
// behind a single ellipsis. Binary search bounds the number of width probes.
static size_t addonCabe(const char *s, float w, TxtEstilo estilo) {
  size_t total=strlen(s);
  if(addonMedir(s,total,estilo)<=w)return total;
  size_t fim[2048], n=0;
  for(size_t i=0;s[i] && n<sizeof fim/sizeof *fim;) {
    i++;while(s[i] && ((unsigned char)s[i]&0xC0)==0x80)i++;
    fim[n++]=i;
  }
  size_t a=0,b=n;
  while(a<b){size_t m=a+(b-a)/2;if(addonMedir(s,fim[m],estilo)<=w)a=m+1;else b=m;}
  if(!a)return 0;
  size_t k=fim[a-1];
  if(s[k]) for(size_t j=k;j>k/2;j--)
    if(strchr(" ._-/|",s[j-1]))return j;
  return k;
}
static void addonQuebrar(const LinhaAddon *linhas,int nl,AddonDesenho *out,int cap,
                         int *quantos,int *visuais,float w,TxtEstilo estilo,int maxLinhas) {
  int row=0,nd=0;
  for(int k=0;k<nl && row<maxLinhas;k++) {
    const LinhaAddon *l=&linhas[k];float x=0;
    for(int pc=0;pc<l->n && row<maxLinhas;pc++) {
      const char *s=pecaAddonTexto(l,pc);size_t inicio=0;
      if(l->ic[pc]) {
        if(x+60>w && x>0){row++;x=0;}
        if(row>=maxLinhas || nd>=cap)goto pronto;
        out[nd++]=(AddonDesenho){0,0,(unsigned char)k,(unsigned char)pc,(unsigned char)row,1,x,22};
        x+=30;
      }
      while(s[inicio] && row<maxLinhas) {
        size_t cabe=addonCabe(s+inicio,w-x,estilo);
        if(!cabe) {
          if(x>0){row++;x=0;continue;}
          // An oversized glyph still makes progress at a complete boundary.
          cabe=1;while(s[inicio+cabe] && ((unsigned char)s[inicio+cabe]&0xC0)==0x80)cabe++;
        }
        if(nd>=cap)goto pronto;
        float larg=(float)addonMedir(s+inicio,cabe,estilo);
        out[nd++]=(AddonDesenho){(unsigned short)inicio,(unsigned short)cabe,
          (unsigned char)k,(unsigned char)pc,(unsigned char)row,0,x,fminf(larg,w-x)};
        inicio+=cabe;x+=larg;
        if(s[inicio]){row++;x=0;while(s[inicio]==' ')inicio++;}
      }
      x+=12;
    }
    row++;
  }
pronto:
  *quantos=nd;*visuais=nd ? (int)out[nd-1].visual+1 : 0;
}
static void textoAddonLimpar(void) {
  free(textoAddon);textoAddon=NULL;textoAddonPreparados=textoAddonRelogio=0;
}
static const TextoAddon *textoAddonDa(int i) {
  static TextoAddon reserva;
  if(i<0 || i>=n)return &reserva;
  if(!textoAddon)textoAddon=calloc(TEXTO_ADDON_CACHE,sizeof *textoAddon);
  TextoAddon *t=&reserva;
  if(textoAddon) {
    // A true bounded LRU avoids collisions when adjacent sorted sources have
    // raw indices 0,32,64,...; such a list must not reparse every frame.
    t=textoAddon;
    for(int k=0;k<TEXTO_ADDON_CACHE;k++) {
      TextoAddon *p=&textoAddon[k];
      if(p->pronto && p->indice==i){t=p;break;}
      if(!p->pronto || (t->pronto && p->uso<t->uso))t=p;
    }
  }
  if(!textoAddon)t->pronto=0; // allocation failure must never reuse another list
  float w=addonTextoW();int corpo,peso,familia=(int)txt_fonte_interface();
  txt_estilo_medidas(TXT_PG_FIM,&corpo,&peso);
  if(!t->pronto || t->indice!=i) {
    memset(t,0,sizeof *t);t->indice=i;t->pronto=1;
    t->nl=linhasAddon(&lista[i],t->linhas,FOLHA_ADDON_LINHAS);
    tituloAddon(&lista[i],t->titulo,sizeof t->titulo);
    const char *nome=lista[i].rotulo[0] ? lista[i].rotulo : lista[i].provedor;
    pecasAddon(nome,strlen(nome),&t->cabecalho);
    textoAddonPreparados++;
  }
  if(++textoAddonRelogio==0) {
    if(textoAddon)for(int k=0;k<TEXTO_ADDON_CACHE;k++)textoAddon[k].uso=0;
    textoAddonRelogio=1;
  }
  t->uso=textoAddonRelogio;
  if(t->largura!=w || t->corpo!=corpo || t->peso!=peso || t->familia!=familia) {
    t->largura=w;t->corpo=corpo;t->peso=peso;t->familia=familia;
    addonQuebrar(t->linhas,t->nl,t->desenhos,ADDON_DESENHOS,&t->nd,&t->nv,w,TXT_PG_FIM,FOLHA_ADDON_VISUAIS);
    int linhas;
    addonQuebrar(&t->cabecalho,1,t->nome,ADDON_PECAS*2,&t->nn,&linhas,w,TXT_CALLOUT,1);
  }
  return t;
}
