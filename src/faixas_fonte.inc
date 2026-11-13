// FROM ANOTHER SOURCE: the pages inside the audio island (included by faixas.c).
//
//  EXP_LISTA   "Best" / "All" (LEFT/RIGHT from any row). Best: the preferred
//              language, the second one, the versions without a language
//              ("Unknown (likely original)"), then one "Other languages" row;
//              All: every version under a header per language.
//  EXP_OUTRAS  Best's "Other languages": a language bar (LEFT/RIGHT) and the
//              versions of the language on it.
//  EXP_FAIXAS  the audio tracks of one version (hold OK on it), read from its
//              header like the source list's "Choose audio track…".
//
// OK uses the version (audioext_escolher) and closes the sheet; Back goes one
// page up. The picture keeps playing all along.
enum { EXP_NADA = 0, EXP_LISTA, EXP_OUTRAS, EXP_FAIXAS };
enum { EXL_CAND = 0, EXL_CAB, EXL_OUTRAS };
#define EX_LIN_MAX (AUDIOEXT_TODOS_MAX * 2 + 4)
#define EX_OUT_MAX 32
#define EX_CAB_H   44.0f
#define EX_SEG_H   54.0f   /* plrui_seg: SEG_IH + 2 * SEG_PAD */
// Other languages with at most this many versions in all show on Best itself.
#define EX_OUTRAS_JUNTO 2
typedef struct { unsigned char tipo; short k; } ExLinha;   // k: index into the page's candidates
static int exPag, exVista, exFoco, exRol, exLingua, exVoltar;
static ExLinha exLin[EX_LIN_MAX];
static int exLinN;
static int exOutIni[EX_OUT_MAX], exOutCont[EX_OUT_MAX], exOutN;   // Best's other languages
static CtxHold exHold;
static unsigned exFxGer;
static int exFxN = -2, exFxFoco;
static AudioExtFaixa exFx[AUDIOEXT_FAIXAS_MAX];
static char exFxRot[AUDIOEXT_FAIXAS_MAX][120];
static AudioExtCand exFxCand;

static const AudioExtCand *exVetor(int *n) {
  if (exPag == EXP_LISTA && exVista == 1) { *n = extAllN; return extAll; }
  *n = extBestN; return extBest;
}
static int exMesmoBalde(const AudioExtCand *a, const AudioExtCand *b) {
  if (a->grupo != b->grupo) return 0;
  return a->grupo != AEXT_G_OUTRA || !strcasecmp(a->idioma, b->idioma) || ling_casa(a->idioma, b->idioma);
}
static int exFocavel(int i) { return i >= 0 && i < exLinN && exLin[i].tipo != EXL_CAB; }
static const AudioExtCand *exCandDaLinha(int i) {
  int n;
  const AudioExtCand *v = exVetor(&n);
  if (i < 0 || i >= exLinN || exLin[i].tipo != EXL_CAND || exLin[i].k < 0 || exLin[i].k >= n) return NULL;
  return &v[exLin[i].k];
}
static void exLinha(int tipo, int k) {
  if (exLinN >= EX_LIN_MAX) return;
  exLin[exLinN].tipo = (unsigned char)tipo; exLin[exLinN].k = (short)k; exLinN++;
}
static void exContarOutras(void) {
  exOutN = 0;
  for (int i = 0; i < extBestN; i++) {
    if (extBest[i].grupo != AEXT_G_OUTRA) continue;
    if (exOutN && exMesmoBalde(&extBest[i], &extBest[exOutIni[exOutN - 1]])) { exOutCont[exOutN - 1]++; continue; }
    if (exOutN >= EX_OUT_MAX) break;
    exOutIni[exOutN] = i; exOutCont[exOutN] = 1; exOutN++;
  }
}
// The rows of the page on screen, from the candidate lists (extMontar). The
// focus stays on the same version when the list grows (add-ons arrive late).
static void exMontarLinhas(int focIdx, int focOutras) {
  exLinN = 0;
  exContarOutras();
  if (exPag == EXP_LISTA && exVista == 0) {
    int principais = 0, outras = 0;
    for (int i = 0; i < extBestN; i++) {
      if (extBest[i].grupo == AEXT_G_OUTRA) { outras++; continue; }
      exLinha(EXL_CAND, i); principais++;
    }
    if (outras && (outras <= EX_OUTRAS_JUNTO || !principais)) {
      for (int i = 0; i < extBestN; i++) if (extBest[i].grupo == AEXT_G_OUTRA) exLinha(EXL_CAND, i);
    } else if (outras) exLinha(EXL_OUTRAS, 0);
  } else if (exPag == EXP_LISTA) {
    for (int i = 0; i < extAllN; i++) {
      if (!i || !exMesmoBalde(&extAll[i], &extAll[i - 1])) exLinha(EXL_CAB, i);
      exLinha(EXL_CAND, i);
    }
  } else if (exPag == EXP_OUTRAS) {
    if (exLingua >= exOutN) exLingua = exOutN - 1;
    if (exLingua < 0) exLingua = 0;
    if (exOutN) for (int i = 0; i < exOutCont[exLingua]; i++) exLinha(EXL_CAND, exOutIni[exLingua] + i);
  }
  // Back on the same version, else the first row that takes the focus.
  exFoco = -1;
  for (int i = 0; i < exLinN && exFoco < 0; i++) {
    const AudioExtCand *c = exCandDaLinha(i);
    if ((c && c->indice == focIdx) || (focOutras && exLin[i].tipo == EXL_OUTRAS)) exFoco = i;
  }
  for (int i = 0; i < exLinN && exFoco < 0; i++) if (exFocavel(i)) exFoco = i;
  if (exFoco < 0) exFoco = 0;
  if (exRol > exFoco) exRol = exFoco;
}
// The version in focus (-1 none), to find it again after a rebuild.
static int exFocoIndice(void) { const AudioExtCand *c = exCandDaLinha(exFoco); return c ? c->indice : -1; }
static int exFocoOutras(void) { return exFoco >= 0 && exFoco < exLinN && exLin[exFoco].tipo == EXL_OUTRAS; }
static void exReconstruir(void) { exMontarLinhas(exFocoIndice(), exFocoOutras()); }
static int exPagAtiva(void) { return exPag != EXP_NADA; }
static float exAreaMax(void) { return IL_VIS * (IL_LN_H + IL_LN_VAO) - IL_LN_VAO; }
static float exAltLinha(int i) { return (exLin[i].tipo == EXL_CAB ? EX_CAB_H : IL_LN_H) + IL_LN_VAO; }
static void exAjustar(void) {
  float area = exAreaMax();
  if (exRol > exFoco) exRol = exFoco;
  // The first row of a language shows its header too.
  if (exRol > 0 && exRol == exFoco && exLin[exRol - 1].tipo == EXL_CAB) exRol--;
  for (;;) {
    float h = 0;
    for (int i = exRol; i <= exFoco && i < exLinN; i++) h += exAltLinha(i);
    if (h - IL_LN_VAO <= area || exRol >= exFoco) break;
    exRol++;
  }
  if (exRol < 0) exRol = 0;
}
static void exIr(int pag) {
  exPag = pag; exRol = 0; exFoco = 0;
  ctxhold_cancelar(&exHold);
  exMontarLinhas(-1, 0);
}
static void exAbrir(void) {
  exVista = 0; exLingua = 0;
  extMontar(1);
  exIr(EXP_LISTA);
  printf("[ext-audio] from another source: opened, best %d, all %d\n", extBestN, extAllN); fflush(stdout);
}
static void exFechar(void) {
  exPag = EXP_NADA;
  ctxhold_cancelar(&exHold);
}
static int exAceitaSegurar(void) {
  return (exPag == EXP_LISTA || exPag == EXP_OUTRAS) && exCandDaLinha(exFoco) && audioext_sonda_disponivel();
}
// "Polski · E-AC-3 5.1 · Dubbing": language, codec and channels, the track
// name when short; "used for sync" on this file's language.
static void exMontarFaixas(void) {
  static const char *const m[][2] = { {"eac3","E-AC-3"}, {"ac3","AC-3"}, {"aac","AAC"}, {"dts","DTS"},
    {"truehd","TrueHD"}, {"opus","Opus"}, {"flac","FLAC"}, {"vorbis","Vorbis"}, {"mp3","MP3"} };
  for (int k = 0; k < exFxN && k < AUDIOEXT_FAIXAS_MAX; k++) {
    const AudioExtFaixa *f = &exFx[k];
    const char *lg = f->idioma[0] ? i18n(ling_nome(f->idioma)) : i18n("Faixa"), *cd = f->codec;
    char ch[8] = "";
    for (size_t j = 0; j < sizeof m / sizeof *m; j++) if (!strcmp(f->codec, m[j][0])) cd = m[j][1];
    if (f->canais == 8) snprintf(ch, sizeof ch, " 7.1");
    else if (f->canais == 6) snprintf(ch, sizeof ch, " 5.1");
    else if (f->canais > 0) snprintf(ch, sizeof ch, " %d.0", f->canais);
    if (f->titulo[0] && strlen(f->titulo) < 40)
      snprintf(exFxRot[k], sizeof exFxRot[k], "%s \xc2\xb7 %s%s \xc2\xb7 %s", lg, cd, ch, f->titulo);
    else snprintf(exFxRot[k], sizeof exFxRot[k], "%s \xc2\xb7 %s%s", lg, cd, ch);
    { const char *ip = audioext_idioma_principal();
      size_t u = strlen(exFxRot[k]);
      if (ip[0] && f->idioma[0] && ling_casa(f->idioma, ip) && u + 4 < sizeof exFxRot[k])
        snprintf(exFxRot[k] + u, sizeof exFxRot[k] - u, " \xc2\xb7 %s", i18n("used for sync")); }
  }
}
static void exAbrirFaixas(void) {
  const AudioExtCand *c = exCandDaLinha(exFoco);
  const Stream *st = c ? extFonteIdx(c->indice) : NULL;
  ctxhold_cancelar(&exHold);
  if (!st || !audioext_sonda_disponivel()) return;
  exFxCand = *c;
  exVoltar = exPag; exPag = EXP_FAIXAS;
  exFxN = -2; exFxFoco = 0;
  exFxGer = audioext_sondar(st->url, st->cabecalhos);
  printf("[ext-audio] from another source: tracks of #%d asked\n", c->indice); fflush(stdout);
}
static void exOk(void) {
  if (exPag == EXP_FAIXAS) {
    if (exFxN > 0 && exFxFoco >= 0 && exFxFoco < exFxN) {
      extUsar(&exFxCand, &exFx[exFxFoco]);
      exFechar(); aberta = 0;
    } else if (exFxN != -2) exPag = exVoltar;
    return;
  }
  if (exFoco < 0 || exFoco >= exLinN) return;
  if (exLin[exFoco].tipo == EXL_OUTRAS) { exLingua = 0; exIr(EXP_OUTRAS); return; }
  { const AudioExtCand *c = exCandDaLinha(exFoco);
    if (!c) return;
    extUsar(c, NULL);
    exFechar(); aberta = 0; }
}
static void exVoltarPagina(void) {
  if (exPag == EXP_FAIXAS) { exPag = exVoltar; return; }
  if (exPag == EXP_OUTRAS) { exPag = EXP_LISTA; exRol = 0; exMontarLinhas(-1, 1); exAjustar(); return; }
  // Back on the audio list, on the "From another source" row.
  exFechar();
  foco[0] = nLinhas(0) - 1;
  ajustarRolagem();
}
static void exTecla(SDL_Keycode k) {
  if (k == SDLK_AC_BACK || k == SDLK_ESCAPE || k == SDLK_BACKSPACE) { exVoltarPagina(); return; }
  if (exPag == EXP_FAIXAS) {
    int n = exFxN > 0 ? exFxN : 1;
    if (k == SDLK_UP && exFxFoco > 0) exFxFoco--;
    else if (k == SDLK_DOWN && exFxFoco < n - 1) exFxFoco++;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) exOk();
    return;
  }
  if (k == SDLK_LEFT || k == SDLK_RIGHT) {
    int d = k == SDLK_RIGHT ? 1 : -1;
    if (exPag == EXP_LISTA) {
      int v = d > 0 ? 1 : 0;
      if (v != exVista) { int idx = exFocoIndice(); exVista = v; exRol = 0; exMontarLinhas(idx, 0); exAjustar(); }
    } else if (exPag == EXP_OUTRAS) {
      int l = exLingua + d;
      if (l >= 0 && l < exOutN) { exLingua = l; exRol = 0; exMontarLinhas(-1, 0); exAjustar(); }
    }
    return;
  }
  if (k == SDLK_UP) { for (int i = exFoco - 1; i >= 0; i--) if (exFocavel(i)) { exFoco = i; break; } exAjustar(); return; }
  if (k == SDLK_DOWN) { for (int i = exFoco + 1; i < exLinN; i++) if (exFocavel(i)) { exFoco = i; break; } exAjustar(); return; }
  if (k == SDLK_RETURN || k == SDLK_KP_ENTER) exOk();
}
static void exEvento(const SDL_Event *e) {
  if (exPag == EXP_LISTA || exPag == EXP_OUTRAS) {
    switch (ctxhold_evento(&exHold, e, exAceitaSegurar())) {
      case CTXH_CONSUMIDO: return;
      case CTXH_TOQUE: exOk(); return;
      case CTXH_LONGO: exAbrirFaixas(); return;
      default: break;
    }
  }
  if (e->type != SDL_KEYDOWN) return;
  exTecla(e->key.keysym.sym);
}
// Per frame: the hold crossing its threshold, and the header read coming back.
static void exPasso(Uint32 agora) {
  if (!aberta) { if (exPag) exFechar(); return; }
  if ((exPag == EXP_LISTA || exPag == EXP_OUTRAS) && ctxhold_passo(&exHold, agora, 1)) exAbrirFaixas();
  if (exPag == EXP_FAIXAS && exFxN == -2) {
    int r = audioext_sonda_resultado(exFxGer, exFx, AUDIOEXT_FAIXAS_MAX);
    if (r != -2) { exFxN = r; exMontarFaixas(); }
  }
  if (exPag == EXP_OUTRAS && !exOutN) exIr(EXP_LISTA);
}

// --- Drawing ------------------------------------------------------------------
static float exConteudoH(void) {
  float h = 0;
  if (exPag == EXP_FAIXAS) {
    int n = exFxN > 0 ? exFxN : 1;
    if (n > IL_VIS) n = IL_VIS;
    return n * IL_LN_H + (n - 1) * IL_LN_VAO;
  }
  if (!exLinN) return 60.0f;
  for (int i = 0; i < exLinN; i++) h += exAltLinha(i);
  h -= IL_LN_VAO;
  return h < exAreaMax() ? h : exAreaMax();
}
static float alturaExt(void) {
  float h = IL_PAD_Y + IL_TIT_H + 14.0f;
  if (exPag == EXP_LISTA || exPag == EXP_OUTRAS) h += EX_SEG_H + 12.0f;
  return h + exConteudoH() + 14.0f + IL_PE_H + IL_PAD_Y;
}
static void exLinhaCand(const AudioExtCand *c, float x, float y, float w, int sel, float a) {
  const Stream *st = extFonteIdx(c->indice);
  char sub[224];
  int dim = 0, cc = sel ? 255 : 219, cs = 122;
  int ativo = st && (audioext_tocando() || audioext_carregando()) && audioext_marcada(st->url);
  float da, dir = x + w - 22.0f;
  extDetalhes(c->indice, c->idioma, sub, sizeof sub, &dim);
  da = dim ? a * 0.5f : a;
  if (sel) plrui_linha_foco((GfxRect){ x, y, w, IL_LN_H }, 22.0f, a);
  rostoIdioma((GfxRect){ x + 22.0f, y + (IL_LN_H - 52.0f) * 0.5f, 52.0f, 52.0f },
              c->idioma[0] ? c->idioma : "?", NULL, sel, da);
  if (ativo) dir -= 46.0f;
  { float tx = x + 22.0f + 52.0f + 18.0f, tw = dir - tx;
    TxtLinha ln = txt_linha_corta(TXT_ILHA_FORTE, extNomeIdioma(c->idioma), cc, cc, cc - 2, 255, tw);
    TxtLinha ls = txt_linha_corta(TXT_ILHA_GENERO, sub, cs, cs, cs - 2, 255, tw);
    float th = (float)ln.h + 5.0f + (float)ls.h, ty = y + (IL_LN_H - th) * 0.5f;
    txt_desenhar_alpha(ln, tx, ty, da);
    txt_desenhar_alpha(ls, tx, ty + (float)ln.h + 5.0f, da); }
  if (ativo) {
    float ar, ag, ab;
    ajustes_acento(&ar, &ag, &ab);
    gfx_icone((GfxRect){ x + w - 22.0f - 28.0f, y + (IL_LN_H - 28.0f) * 0.5f, 28.0f, 28.0f }, "pl_check", ar, ag, ab, a);
  }
}
// The header of a language on All: its name, and the count on the right.
static void exLinhaCab(int k, float x, float y, float w, float a) {
  int n, cont = 0;
  const AudioExtCand *v = exVetor(&n);
  char nome[96];
  if (k < 0 || k >= n) return;
  for (int i = k; i < n && exMesmoBalde(&v[i], &v[k]); i++) cont++;
  if (v[k].grupo == AEXT_G_SEM) snprintf(nome, sizeof nome, "%s", i18n("Unknown (likely original)"));
  else snprintf(nome, sizeof nome, "%s", i18n(ling_nome(v[k].idioma)));
  { TxtLinha l = txt_linha(TXT_ILHA_APOIO, nome, 243, 242, 239, 140);
    txt_desenhar_alpha(l, x + 22.0f, y + EX_CAB_H - (float)l.h - 8.0f, a); }
  { char q[16]; snprintf(q, sizeof q, "%d", cont);
    TxtLinha l = txt_linha(TXT_ILHA_APOIO, q, 243, 242, 239, 115);
    txt_desenhar_alpha(l, x + w - 22.0f - (float)l.w, y + EX_CAB_H - (float)l.h - 8.0f, a); }
}
static void exLinhaOutras(float x, float y, float w, int sel, float a) {
  char sub[128] = "", q[16];
  int total = 0, cc = sel ? 255 : 219, cs = 122;
  float yc = y + IL_LN_H * 0.5f, dir = x + w - 22.0f;
  for (int i = 0; i < exOutN; i++) {
    total += exOutCont[i];
    if (i < 6) extJuntar(sub, sizeof sub, ling_selo(extBest[exOutIni[i]].idioma));
  }
  if (exOutN > 6) extJuntar(sub, sizeof sub, "\xe2\x80\xa6");
  if (sel) plrui_linha_foco((GfxRect){ x, y, w, IL_LN_H }, 22.0f, a);
  rostoIdioma((GfxRect){ x + 22.0f, y + (IL_LN_H - 52.0f) * 0.5f, 52.0f, 52.0f }, NULL, "aj_languages", sel, a);
  gfx_icone((GfxRect){ dir - 24.0f, yc - 12.0f, 24.0f, 24.0f }, "pl_chevron-right", 1, 1, 1, (sel ? 1.0f : 0.55f) * a);
  dir -= 24.0f + 10.0f;
  snprintf(q, sizeof q, "%d", total);
  { TxtLinha lv = txt_linha(TXT_G18R, q, 243, 242, 239, 115);
    txt_desenhar_alpha(lv, dir - (float)lv.w, yc - (float)lv.h * 0.5f, a);
    dir -= (float)lv.w + 14.0f; }
  { float tx = x + 22.0f + 52.0f + 18.0f, tw = dir - tx;
    TxtLinha ln = txt_linha_corta(TXT_ILHA_FORTE, i18n("Other languages"), cc, cc, cc - 2, 255, tw);
    TxtLinha ls = txt_linha_corta(TXT_ILHA_GENERO, sub, cs, cs, cs - 2, 255, tw);
    float th = (float)ln.h + 5.0f + (float)ls.h, ty = y + (IL_LN_H - th) * 0.5f;
    txt_desenhar_alpha(ln, tx, ty, a);
    txt_desenhar_alpha(ls, tx, ty + (float)ln.h + 5.0f, a); }
}
// The language bar: the window of languages around the one picked that fits.
static void exBarraLinguas(float x, float y, float w, float a) {
  const char *rot[8];
  int cont[8], ini = exLingua, fim = exLingua + 1;
  if (!exOutN) return;
  // Grow the window around the pick while it fits (8 at most, plrui_seg).
  for (int cresceu = 1; cresceu;) {
    cresceu = 0;
    for (int lado = 0; lado < 2; lado++) {
      int a0 = ini - (lado == 0), a1 = fim + (lado == 1);
      if (a0 < 0 || a1 > exOutN || a1 - a0 > 8) continue;
      for (int i = a0; i < a1; i++) { rot[i - a0] = i18n(ling_nome(extBest[exOutIni[i]].idioma)); cont[i - a0] = exOutCont[i]; }
      if (plrui_seg(rot, cont, a1 - a0, (float)(exLingua - a0), 0, -1.0f, 0, a) <= w) { ini = a0; fim = a1; cresceu = 1; }
    }
  }
  for (int i = ini; i < fim; i++) { rot[i - ini] = i18n(ling_nome(extBest[exOutIni[i]].idioma)); cont[i - ini] = exOutCont[i]; }
  plrui_seg(rot, cont, fim - ini, (float)(exLingua - ini), 0, x, y, a);
}
static void corpoExt(GfxRect c, float a) {
  float x0 = c.x + IL_PAD_X, w = c.w - IL_PAD_X * 2.0f, y = c.y + IL_PAD_Y;
  char q[96];
  const char *titulo = exPag == EXP_OUTRAS ? "Other languages" : exPag == EXP_FAIXAS ? "Choose audio track…" : "From another source";
  { TxtLinha t = txt_linha(TXT_ILHA_PERGUNTA, titulo, 243, 242, 239, 255);
    txt_desenhar_alpha(t, x0 + 10.0f, y, a);
    if (exPag == EXP_OUTRAS) snprintf(q, sizeof q, i18n("%d languages"), exOutN);
    else if (exPag == EXP_FAIXAS) snprintf(q, sizeof q, "%s", extNomeIdioma(exFxCand.idioma));
    else snprintf(q, sizeof q, i18n("%d versions"), extAllN);
    { TxtLinha l = txt_linha_corta(TXT_G18R, q, 243, 242, 239, 115, w * 0.45f);
      txt_desenhar_alpha(l, x0 + w - 10.0f - l.w, y + (float)t.h - (float)l.h - 8.0f, a); } }
  y += IL_TIT_H + 14.0f;
  if (exPag == EXP_LISTA) {
    const char *rot[2] = { "Best", "All" };
    int cont[2] = { extBestN, extAllN };
    plrui_seg(rot, cont, 2, (float)exVista, 0, x0 + 10.0f, y, a);
    y += EX_SEG_H + 12.0f;
  } else if (exPag == EXP_OUTRAS) {
    exBarraLinguas(x0 + 10.0f, y, w - 20.0f, a);
    y += EX_SEG_H + 12.0f;
  }
  { float area = exConteudoH(), y0 = y;
    if (exPag == EXP_FAIXAS) {
      int n = exFxN > 0 ? exFxN : 1, r = 0, vis = IL_VIS;
      if (exFxFoco >= vis) r = exFxFoco - vis + 1;
      for (int i = r; i < n && i < r + vis; i++, y += IL_LN_H + IL_LN_VAO) {
        int sel = i == exFxFoco, cc = sel ? 255 : 219;
        const char *rotulo = exFxN == -2 ? "Reading the audio tracks…"
                           : exFxN == 0 ? "No audio track this TV can play"
                           : exFxN < 0 ? "Could not read the audio tracks" : exFxRot[i];
        if (sel) plrui_linha_foco((GfxRect){ x0, y, w, IL_LN_H }, 22.0f, a);
        rostoIdioma((GfxRect){ x0 + 22.0f, y + (IL_LN_H - 52.0f) * 0.5f, 52.0f, 52.0f }, NULL,
                    exFxN > 0 || exFxN == -2 ? "pl_audio-lines" : "aj_triangle-alert", sel, a);
        { TxtLinha l = txt_linha_corta(TXT_ILHA_FORTE, rotulo, cc, cc, cc - 2, 255, w - 92.0f - 22.0f);
          txt_desenhar_alpha(l, x0 + 92.0f, y + (IL_LN_H - (float)l.h) * 0.5f, a); }
      }
    } else if (!exLinN) {
      txt_bloco(TXT_ILHA_TEXTO, "No other version of this title can lend its audio.", 160, 160, 158,
                x0 + 10.0f, y + 12.0f, w - 20.0f, 28, a, 2);
    } else {
      exAjustar();
      for (int i = exRol; i < exLinN; i++) {
        float h = exAltLinha(i) - IL_LN_VAO;
        if (y + h > y0 + area + 0.5f) break;
        // A header goes with its first row: not alone at the bottom.
        if (exLin[i].tipo == EXL_CAB && i + 1 < exLinN && y + h + IL_LN_VAO + IL_LN_H > y0 + area + 0.5f) break;
        if (exLin[i].tipo == EXL_CAB) exLinhaCab(exLin[i].k, x0, y, w, a);
        else if (exLin[i].tipo == EXL_OUTRAS) exLinhaOutras(x0, y, w, i == exFoco, a);
        else { const AudioExtCand *cd = exCandDaLinha(i); if (cd) exLinhaCand(cd, x0, y, w, i == exFoco, a); }
        y += h + IL_LN_VAO;
      }
    }
    y = y0 + area + 14.0f; }
  // FOOTER: the position, and the keys of this page.
  gfx_cor((GfxRect){ x0, y, w, 1.0f }, 0.0f, 1, 1, 1, 0.07f * a);
  { float yc = y + 16.0f + 15.0f;
    int pos = 0, tot = 0;
    if (exPag == EXP_FAIXAS) { pos = exFxFoco + 1; tot = exFxN > 0 ? exFxN : 0; }
    else for (int i = 0; i < exLinN; i++) if (exFocavel(i)) { tot++; if (i <= exFoco) pos = tot; }
    snprintf(q, sizeof q, i18n("%d de %d"), tot ? pos : 0, tot);
    { TxtLinha l = txt_linha(TXT_ILHA_APOIO, q, 243, 242, 239, 115);
      txt_desenhar_alpha(l, x0 + 10.0f, yc - (float)l.h * 0.5f, a); }
    if (exPag == EXP_FAIXAS) {
      const char *k[2] = { "OK", "Voltar" }, *rt[2] = { "Use", "Cancel" };
      plrui_dicas(k, rt, 2, x0 + w - 10.0f, yc, 1, a);
    } else {
      // Back (one page up) is the remote's own: the three keys of this page.
      const char *lado = exPag == EXP_OUTRAS ? "Language" : "Best / All";
      if (exFocoOutras() || !exCandDaLinha(exFoco) || !audioext_sonda_disponivel()) {
        const char *k[2] = { "\xe2\x86\x90 \xe2\x86\x92", "OK" };
        const char *rt[2] = { lado, exFocoOutras() ? "Open" : "Use" };
        plrui_dicas(k, rt, 2, x0 + w - 10.0f, yc, 1, a);
      } else {
        // Long languages do not fit the three: the bar above already shows
        // LEFT/RIGHT's views, so that hint goes first, then the hold's.
        const char *k[3] = { "\xe2\x86\x90 \xe2\x86\x92", "OK", "Hold OK" };
        const char *rt[3] = { lado, "Use", "Pick track" };
        float livre = w - 20.0f - (float)txt_largura(TXT_ILHA_APOIO, q) - 24.0f;
        int ini = 0, n = 3;
        if (plrui_dicas(k, rt, 3, -1.0f, yc, 1, a) > livre) { ini = 1; n = 2; }   /* x < 0: measured only */
        if (n == 2 && plrui_dicas(k + 1, rt + 1, 2, -1.0f, yc, 1, a) > livre) n = 1;
        plrui_dicas(k + ini, rt + ini, n, x0 + w - 10.0f, yc, 1, a);
      }
    } }
}
