// Rich addon cards. Parsing and text measurement are lazy, bounded and kept
// outside scroll animation. Unvisited rows use cheap newline counts; visited
// rows retain only their measured height, independently of the bounded LRU.
#define FOLHA_ADDON_LINHAS 12
#define FOLHA_ADDON_VISUAIS 96
#define FOLHA_ADDON_LD 28.0f
#define FOLHA_ADDON_TOPO 20.0f
#define FOLHA_ADDON_SELOS 36.0f
#define FOLHA_ADDON_TITULO 40.0f
#define FOLHA_ADDON_BASE 18.0f
#define FOLHA_ADDON_MARCA FOLHA_MARCA_H
#define FOLHA_ADDON_GAP 14.0f
#define ADDON_PECAS 24
#define ADDON_DESENHOS 256
#define TEXTO_ADDON_CACHE 32
// FORMATTER ICONS (tools/icones-formatador.py, from Namer03's Formatter set):
// one entry per emoji the stream formatters print. Standard = Twemoji in colour
// (tw_) or a monochrome symbol drawn in the text colour (sy_); Minimalistic =
// Lucide in the text colour. Country flags are Twemoji too (fl_XX) in
// Standard; in Minimalistic they stay text and nv_limpar_texto writes the
// language code. Emoji outside the table stay text, as the UI font draws them.
typedef struct { const char *utf8, *padrao, *minimalista; int cor; } FmtIcone;
static const FmtIcone FMT_ICONES[] = {
#include "formatador_icones.inc"
};
#define FMT_ICONES_N ((int)(sizeof FMT_ICONES / sizeof *FMT_ICONES))
// What a piece's icon is drawn with: tinted with the text colour, or the PNG's
// own colours (colour emoji and flags). Every icon is square (22x22).
enum { ADDON_IC_TOM = 0, ADDON_IC_COR = 1 };

typedef struct {
  int n;
  const char *ic[ADDON_PECAS];
  const char *alt[ADDON_PECAS];      // drawn tinted when the colour PNG is missing
  unsigned char tipo[ADDON_PECAS];   // ADDON_IC_*
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
  int nl, nv, nd, nn, indice, pronto, estilo;
  int corpo, peso, familia;
  unsigned uso;
  float largura;
} TextoAddon;
static TextoAddon *textoAddon;
static unsigned textoAddonPreparados; // regression instrumentation, no user data
static unsigned textoAddonRelogio;
static unsigned short *addonVisuais; // 0 unknown, otherwise visual rows + 1
static int addonVisuaisCap;
static unsigned addonMedidasRevisao;
static unsigned addonEstiloRevisao;
// Only newly measured cards need new heights. Overflow is an exceptional
// bulk-measurement path and requests one complete geometry rebuild.
static int addonMedidos[128], addonMedidosN, addonMedidosTodos;
static void addonMedido(int i) {
  for(int k=0;k<addonMedidosN;k++)if(addonMedidos[k]==i)return;
  if(addonMedidosN<(int)(sizeof addonMedidos/sizeof *addonMedidos))addonMedidos[addonMedidosN++]=i;
  else addonMedidosTodos=1;
}
static float addonMedidasW;
static int addonMedidasCorpo, addonMedidasPeso, addonMedidasFamilia, addonMedidasEstilo;

static void linhaLimpa(char *d, size_t tam, const char *ini, size_t n) {
  size_t k = 0;
  while (n && (*ini == ' ' || *ini == '\t')) { ini++; n--; }
  while (n && (ini[n-1] == ' ' || ini[n-1] == '\t' || ini[n-1] == '\r')) n--;
  while (k < n && k + 1 < tam) { unsigned char c = (unsigned char)ini[k]; d[k++] = c < 32 ? ' ' : (char)c; }
  d[k] = 0;
}
// Strict UTF-8 decoder (as limpa.c): an invalid sequence is U+FFFD, 1 byte.
static unsigned long fmtDec(const unsigned char *p, int *n) {
  unsigned long cp;
  int len, i;
  if (*p < 0x80) { *n = 1; return *p; }
  if (*p >= 0xC2 && *p <= 0xDF) { len = 2; cp = *p & 0x1F; }
  else if (*p >= 0xE0 && *p <= 0xEF) { len = 3; cp = *p & 0x0F; }
  else if (*p >= 0xF0 && *p <= 0xF4) { len = 4; cp = *p & 0x07; }
  else { *n = 1; return 0xFFFD; }
  for (i = 1; i < len; i++) {
    if ((p[i] & 0xC0) != 0x80) { *n = 1; return 0xFFFD; }
    cp = cp << 6 | (p[i] & 0x3F);
  }
  if ((len == 3 && cp < 0x800) || (len == 4 && (cp < 0x10000 || cp > 0x10FFFF)) ||
      (cp >= 0xD800 && cp <= 0xDFFF)) { *n = 1; return 0xFFFD; }
  *n = len;
  return cp;
}
static int fmtVS(unsigned long c) { return c == 0xFE0F || c == 0xFE0E; }
static int fmtTom(unsigned long c) { return c >= 0x1F3FB && c <= 0x1F3FF; }
// After an icon, variation selectors, skin tones and the rest of an unknown
// ZWJ sequence go with it: "🕵️‍♀️" must not leave a loose "♀" behind.
static const unsigned char *fmtCauda(const unsigned char *q) {
  for (;;) {
    int n, n2;
    unsigned long c;
    if (!*q) return q;
    c = fmtDec(q, &n);
    if (fmtVS(c) || fmtTom(c)) { q += n; continue; }
    if (c == 0x200D && q[n]) {
      unsigned long c2 = fmtDec(q + n, &n2);
      if (c2 >= 0x2000 && c2 != 0xFFFD) { q += n + n2; continue; }
    }
    return q;
  }
}
// Table entry at the start of `p` (longest sequences come first in the table),
// tolerant of VS15/VS16 and skin tones the table does not have. -1 = none.
// Ordinary ASCII filenames never search the table.
static int emojiIcone(const char *p, size_t *len) {
  const unsigned char *s = (const unsigned char *)p;
  if (!s || *s < 0xC2) return -1;
  for (int k = 0; k < FMT_ICONES_N; k++) {
    const unsigned char *e = (const unsigned char *)FMT_ICONES[k].utf8, *q = s;
    int ok = 1;
    if (*e != *s) continue;
    while (*e) {
      int ne, nq = 1;
      unsigned long ce = fmtDec(e, &ne), cq = 0;
      for (;;) {
        if (!*q) { ok = 0; break; }
        cq = fmtDec(q, &nq);
        if (fmtVS(cq) || (fmtTom(cq) && !fmtTom(ce))) { q += nq; continue; }
        break;
      }
      if (!ok || cq != ce) { ok = 0; break; }
      e += ne; q += nq;
    }
    if (!ok) continue;
    q = fmtCauda(q);
    if (len) *len = (size_t)(q - s);
    return k;
  }
  return -1;
}
// Flag at the start of `s`: a pair of regional indicators ("GB") or 🏴 with the
// tags of England, Scotland and Wales ("GB-ENG"). 0 = not a flag.
static size_t bandeiraEm(const unsigned char *s, char cc[8]) {
  int n1, n2;
  unsigned long a, b;
  if (!s || *s < 0xC2) return 0;
  a = fmtDec(s, &n1);
  if (a >= 0x1F1E6 && a <= 0x1F1FF && s[n1]) {
    b = fmtDec(s + n1, &n2);
    if (b >= 0x1F1E6 && b <= 0x1F1FF) {
      cc[0] = (char)('A' + (a - 0x1F1E6)); cc[1] = (char)('A' + (b - 0x1F1E6)); cc[2] = 0;
      return (size_t)(n1 + n2);
    }
    return 0;
  }
  if (a == 0x1F3F4) {
    static const struct { const char *tag, *cc; } SUB[] = {
      { "gbeng", "GB-ENG" }, { "gbsct", "GB-SCT" }, { "gbwls", "GB-WLS" } };
    char tag[8];
    int t = 0;
    const unsigned char *q = s + n1;
    for (;;) {
      int n;
      unsigned long c;
      if (!*q) return 0;
      c = fmtDec(q, &n);
      if (c == 0xE007F) { q += n; break; }
      if (c < 0xE0020 || c > 0xE007E || t >= 7) return 0;
      tag[t++] = (char)(c - 0xE0000);
      q += n;
    }
    tag[t] = 0;
    for (unsigned i = 0; i < sizeof SUB / sizeof *SUB; i++)
      if (!strcmp(tag, SUB[i].tag)) { strcpy(cc, SUB[i].cc); return (size_t)(q - s); }
  }
  return 0;
}
// "formatador/fl_XX" for the flags the package has, interned once per code:
// the piece keeps a pointer, and the disk is asked at parse time only, never
// per frame. NULL = no PNG (the flag stays text, as the language code).
static const char *bandeiraNome(const char *cc) {
  static struct { char cc[8], nome[32]; int existe; } memo[320];
  static int nMemo;
  int k;
  for (k = 0; k < nMemo; k++)
    if (!strcmp(memo[k].cc, cc)) return memo[k].existe ? memo[k].nome : NULL;
  if (nMemo == (int)(sizeof memo / sizeof *memo)) return NULL;
  snprintf(memo[nMemo].cc, sizeof memo[nMemo].cc, "%s", cc);
  snprintf(memo[nMemo].nome, sizeof memo[nMemo].nome, "formatador/fl_%s", cc);
  memo[nMemo].existe = gfx_icone_existe(memo[nMemo].nome);
  if (memo[nMemo].existe < 0) return NULL;   // art folder not set yet: ask again later
  nMemo++;
  return memo[nMemo - 1].existe ? memo[nMemo - 1].nome : NULL;
}
// The icon at the start of `p` in the current style. Returns the consumed
// length (0 = no icon here) and fills the drawing name, fallback and kind.
static size_t iconeAddon(const char *p, int minimalista, const char **ic,
                         const char **alt, unsigned char *tipo) {
  size_t len = 0;
  char cc[8];
  int k;
  if ((unsigned char)*p < 0xC2) return 0;
  if ((len = bandeiraEm((const unsigned char *)p, cc))) {
    const char *nome = minimalista ? NULL : bandeiraNome(cc);
    if (!nome) return 0;   // text: nv_limpar_texto writes the language code
    *ic = nome; *alt = NULL; *tipo = ADDON_IC_COR;
    return len;
  }
  k = emojiIcone(p, &len);
  if (k < 0) return 0;
  // Standard artwork missing from the package (an old art folder): the
  // Lucide icon, tinted, instead of an empty gap. Asked once per entry.
  static signed char existe[FMT_ICONES_N];
  if (!minimalista && !existe[k]) {
    int e = gfx_icone_existe(FMT_ICONES[k].padrao);
    if (e >= 0) existe[k] = e ? 1 : -1;   // e < 0: art folder not set yet, ask again
  }
  if (minimalista || existe[k] < 0) {
    *ic = FMT_ICONES[k].minimalista; *alt = NULL; *tipo = ADDON_IC_TOM;
  } else {
    *ic = FMT_ICONES[k].padrao; *alt = NULL;
    *tipo = FMT_ICONES[k].cor ? ADDON_IC_COR : ADDON_IC_TOM;
  }
  return len;
}
static const char *pecaAddonTexto(const LinhaAddon *l, int p) { return l->texto + l->off[p]; }
static void pecasAddon(const char *ini, size_t n, LinhaAddon *l) {
  char cru[2048]; size_t usado = 0;
  int minimalista = ajustes_fonte_icones_minimalista();
  memset(l, 0, sizeof *l);
  linhaLimpa(cru, sizeof cru, ini, n);
  for (const char *p = cru; *p && l->n < ADDON_PECAS && usado + 1 < sizeof l->texto;) {
    const char *icone = NULL, *alt = NULL; unsigned char tipo = ADDON_IC_TOM;
    size_t el = iconeAddon(p, minimalista, &icone, &alt, &tipo);
    const char *q = p + el;
    const char *fim = q;
    while (*fim) {
      const char *zi, *za; unsigned char zt;
      if (iconeAddon(fim, minimalista, &zi, &za, &zt)) break;
      fim++;
    }
    char seg[2048]; size_t tamanho = (size_t)(fim-q);
    if (tamanho >= sizeof seg) tamanho = sizeof seg-1;
    memcpy(seg,q,tamanho); seg[tamanho]=0;
    l->off[l->n] = (unsigned short)usado;
    nv_limpar_texto(seg,l->texto+usado,sizeof l->texto-usado,NV_LIMPA_UMA_LINHA);
    nv_aparar_separadores(l->texto+usado);
    if (icone || l->texto[usado]) {
      l->alt[l->n] = alt; l->tipo[l->n] = tipo;
      l->ic[l->n++] = icone;
      usado += strlen(l->texto+usado)+1;
    }
    p = fim;
  }
}
// AIOStreams templates can remove this display-only library prefix from the
// description. Compare the same filename without changing playback metadata.
static const char *addonArquivoComparar(const char *nome) {
  if(!strncmp(nome,"[Cloud]",7)) {
    nome+=7;
    while(*nome==' ' || *nome=='\t')nome++;
  }
  return nome;
}
static int linhasAddon(const Stream *s, LinhaAddon *out, int max) {
  int nl = 0, arquivoVisto = 0;
  const char *comparar=addonArquivoComparar(s->arquivo);
  const char *arq = comparar[0] ? strstr(s->descricao,comparar) : NULL;
  for (const char *p = s->descricao; *p && nl < max;) {
    const char *f = strchr(p,'\n');
    if (arq && arq >= p && (!f || arq < f)) arquivoVisto = 1;
    pecasAddon(p,f ? (size_t)(f-p) : strlen(p),&out[nl]);
    if (out[nl].n) nl++;
    if (!f) break;
    p=f+1;
  }
  if(max>0 && s->arquivo[0] && !arquivoVisto) {
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
  const char *comparar=addonArquivoComparar(s->arquivo);
  if(nl<FOLHA_ADDON_LINHAS && s->arquivo[0] && (!comparar[0] || !strstr(s->descricao,comparar)))nl++;
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
static void addonMedidasAtualizar(void) {
  int corpo,peso,familia=(int)txt_fonte_interface();
  float w=addonTextoW();txt_estilo_medidas(TXT_PG_FIM,&corpo,&peso);
  if(n>addonVisuaisCap) {
    unsigned short *p=realloc(addonVisuais,(size_t)n*sizeof *p);
    if(p) {
      memset(p+addonVisuaisCap,0,(size_t)(n-addonVisuaisCap)*sizeof *p);
      addonVisuais=p;addonVisuaisCap=n;
    }
  }
  // The icon style changes pieces (a flag is an icon or a language code), so
  // every measured height is stale with it, like with a new width or font.
  int estilo=ajustes_fonte_icones_minimalista();
  if(w!=addonMedidasW || corpo!=addonMedidasCorpo || peso!=addonMedidasPeso || familia!=addonMedidasFamilia ||
     estilo!=addonMedidasEstilo) {
    if(addonVisuais)memset(addonVisuais,0,(size_t)addonVisuaisCap*sizeof *addonVisuais);
    addonMedidasW=w;addonMedidasCorpo=corpo;addonMedidasPeso=peso;addonMedidasFamilia=familia;
    addonMedidasEstilo=estilo;
    addonMedidasRevisao++;
    addonEstiloRevisao++;
  }
}
static int addonLinhasVisuais(int i) {
  return i<addonVisuaisCap && addonVisuais[i] ? addonVisuais[i]-1 : linhasAddonContar(&lista[i]);
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
typedef struct {
  char texto[2048];
  unsigned short off[2048], tam[2048];
  int n, corpo, peso, familia;
  float largura;
} ArquivoLayout;
static ArquivoLayout arqLayout[2];
static const ArquivoLayout *arquivoLayout(const char *src) {
  int corpo,peso,familia=(int)txt_fonte_interface();
  float w=addonTextoW(); txt_estilo_medidas(TXT_PG_FIM,&corpo,&peso);
  for(int k=0;k<2;k++) if(arqLayout[k].largura==w && arqLayout[k].corpo==corpo &&
      arqLayout[k].peso==peso && arqLayout[k].familia==familia && !strcmp(arqLayout[k].texto,src)) return &arqLayout[k];
  static unsigned turno; ArquivoLayout *a=&arqLayout[turno++%2];
  memset(a,0,sizeof *a);snprintf(a->texto,sizeof a->texto,"%s",src);
  a->largura=w;a->corpo=corpo;a->peso=peso;a->familia=familia;
  size_t off=0;
  while(a->texto[off] && a->n<2048) {
    size_t n=addonCabe(a->texto+off,w,TXT_PG_FIM);
    if(!n){n=1;while(a->texto[off+n] && ((unsigned char)a->texto[off+n]&0xC0)==0x80)n++;}
    a->off[a->n]=(unsigned short)off;a->tam[a->n++]=(unsigned short)n;off+=n;
  }
  return a;
}
static float arquivoAltura(const char *s) { return s[0] ? arquivoLayout(s)->n*FOLHA_ARQ_H : 0; }
static void arquivoDesenhar(const char *s,float x,float y,float alpha) {
  const ArquivoLayout *a=arquivoLayout(s);
  for(int k=0;k<a->n;k++) {
    char parte[2048];memcpy(parte,a->texto+a->off[k],a->tam[k]);parte[a->tam[k]]=0;
    txt_desenhar_alpha(txt_linha(TXT_PG_FIM,parte,125,125,123,255),x,y+k*FOLHA_ARQ_H,alpha);
  }
}
static void addonQuebrar(const LinhaAddon *linhas,int nl,AddonDesenho *out,int cap,
                         int *quantos,int *visuais,float w,TxtEstilo estilo,int maxLinhas) {
  int row=0,nd=0;
  for(int k=0;k<nl && row<maxLinhas;k++) {
    const LinhaAddon *l=&linhas[k];float x=0;
    for(int pc=0;pc<l->n && row<maxLinhas;pc++) {
      const char *s=pecaAddonTexto(l,pc);size_t inicio=0;
      if(l->ic[pc]) {
        float iw=22;
        if(x+60>w && x>0){row++;x=0;}
        if(row>=maxLinhas || nd>=cap)goto pronto;
        out[nd++]=(AddonDesenho){0,0,(unsigned char)k,(unsigned char)pc,(unsigned char)row,1,x,iw};
        x+=iw+8;
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
  free(addonVisuais);addonVisuais=NULL;addonVisuaisCap=0;addonMedidasRevisao++;
  addonMedidosN=addonMedidosTodos=0;
}
static const TextoAddon *textoAddonDa(int i) {
  static TextoAddon reserva;
  if(i<0 || i>=n)return &reserva;
  addonMedidasAtualizar();
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
  int estilo=ajustes_fonte_icones_minimalista();
  if(!t->pronto || t->indice!=i || t->estilo!=estilo) {
    memset(t,0,sizeof *t);t->indice=i;t->pronto=1;t->estilo=estilo;
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
  if(i<addonVisuaisCap && addonVisuais[i]!=t->nv+1) {
    addonVisuais[i]=(unsigned short)(t->nv+1);addonMedidasRevisao++;
    addonMedido(i);
  }
  return t;
}
