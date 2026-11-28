// The 2.0.9.2 release card: what this build adds over 2.0.8, the 2.0.9.1
// changes included. Text keys in Portuguese, translated in idioma_*.h. Keep it
// up to date with every change of the build.
static void visual2092(GfxRect p, float t, float a, int c);
static void visual2092a(GfxRect p, float t, float a) { visual2092(p, t, a, 0); }
static void visual2092b(GfxRect p, float t, float a) { visual2092(p, t, a, 1); }
static void visual2092c(GfxRect p, float t, float a) { visual2092(p, t, a, 2); }
static void visual2092d(GfxRect p, float t, float a) { visual2092(p, t, a, 3); }
static const NovItem ITENS_2092[] = {
  { .grupo = "Destaques", .icone = "aj_captions", .nome = "Novo motor de legendas",
    .frase = "As legendas são desenhadas pelo libass: ASS com estilo mantém fontes, cores, posições e Signs & Songs, no relógio do próprio player. Plus Jakarta Sans é a fonte padrão, e a lista de legendas fica agrupada por idioma.", .linhas = 9, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_link", .nome = "Legendas queimadas do TorBox",
    .frase = "Com uma chave TorBox Pro, uma faixa ASS embutida toca queimada na imagem pelo TorBox, no quadro exato. Sincronia, retomada e capítulos ficam no arquivo original, que também toca se o TorBox não conseguir.", .linhas = 8, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_audio-lines", .nome = "Áudio de outra fonte",
    .frase = "Mantenha a imagem de uma fonte e toque o áudio de outra versão do mesmo título, como uma dublagem. Uma página lista cada versão por idioma, e os próximos episódios mantêm a mesma versão e faixa.", .linhas = 10, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_rotate-cw", .nome = "Sincronia sozinha",
    .frase = "As legendas posicionam a outra versão na hora, e depois o áudio dela confere. A sincronia é conferida durante todo o episódio e corrigida em momentos de silêncio. Em Sync áudio, Scan again roda tudo de novo e deixa escolher a faixa de comparação numa lista que rola.", .linhas = 12, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_monitor-play", .nome = "HLS (experimental)",
    .frase = "Toca as conversões de DTS e Dolby Vision P7 por HLS. A TV escolhe fMP4 ou MPEG-TS sozinha.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_tv-minimal-play", .nome = "Trailers do Trailerio",
    .frase = "Os trailers automáticos tentam o Trailerio depois da Apple TV e do IMDb, e Esquerda e Direita avançam 5 s.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_play", .nome = "Continuar assistindo",
    .frase = "Selos de Novo episódio e Nova temporada, episódios recém-exibidos vão para a frente, e a retomada abre o episódio na hora com a fonte e o áudio dele.", .linhas = 7 },
  { .grupo = "Mais novidades", .icone = "aj_rotate-cw", .nome = "Outra montagem",
    .frase = "Quando a outra versão é uma montagem diferente, a sincronia começa pelo deslocamento atual, procura no arquivo inteiro por partes, e uma cena longa que falta nela não a interrompe mais. Versões com a mesma duração aparecem primeiro, e uma versão escolhida de novo volta ao último deslocamento.", .linhas = 10, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_play", .nome = "Reprodução automática com debrid",
    .frase = "Torrents que o addon diz fora do cache são perguntados ao TorBox ou ao Premiumize, e a reprodução automática não termina mais em nenhuma fonte funciona. Os que estão no cache sobem na lista.", .linhas = 7, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_link", .nome = "Plano do TorBox",
    .frase = "O plano é lido da sua conta TorBox quando a chave é inserida, e Testar chave do TorBox mostra qual é. Sem escolha manual.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_headphones", .nome = "DTS-HD no menu de áudio",
    .frase = "DTS-HD e DTS-HD MA aparecem separados do DTS simples, também com o DTS desligado nos Ajustes.", .linhas = 4, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_headphones", .nome = "DTS de outra fonte",
    .frase = "Com a conversão de DTS desligada, uma TV que toca DTS recebe as faixas DTS da outra versão como estão.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_puzzle", .nome = "Ícones das fontes",
    .frase = "As fontes Do addon desenham emoji, bandeiras e símbolos como ícones, no estilo Padrão ou Minimalista. Uma nota em estrelas mostra cinco estrelas iguais.", .linhas = 7 },
  { .icone = "aj_list-video", .nome = "Capítulos e pulos",
    .frase = "Capítulos MKV são lidos como no NuvioTV, com pulos de abertura, recapitulação, créditos e pós-créditos do IntroDB, Anime-Skip e AniSkip. Escolher um capítulo ao arrastar busca uma vez só, e o nome do capítulo fica logo abaixo da barra.", .linhas = 9 },
  { .icone = "aj_search", .nome = "Cursor da busca",
    .frase = "O cursor de texto na Busca, no Destaque e no teclado ficou mais fácil de ver.", .linhas = 4 },
  { .icone = "aj_rotate-ccw-clock", .nome = "Volta depois do Home ou do repouso",
    .frase = "Saia com o Home ou deixe a TV dormir: de volta ao app, o filme reabre pausado no ponto exato, e títulos com DTS e Dolby Vision voltam pelo próprio caminho.", .linhas = 8, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_film", .nome = "Filmes em partes",
    .frase = "Nos créditos de uma parte, uma contagem para a Parte N de M com Começar agora ou Continuar assistindo. Um link da próxima parte que esperou demais é buscado de novo.", .linhas = 7 },
  { .icone = "aj_film", .nome = "Miniaturas da barra de busca",
    .frase = "A TV faz as miniaturas da barra de busca sozinha, a partir do arquivo, enquanto toca: sem chave, qualquer título. Cima na barra abre a fileira de capítulos com uma imagem para cada capítulo.", .linhas = 8, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_monitor-play", .nome = "Dolby Vision P7 para P8.1",
    .frase = "Uma chave converte o perfil 7 do Dolby Vision em MKV e MP4, sem a marca de experimental. Desligada, um MP4 Dolby Vision que a TV recusa ainda toca em HDR10, também com áudio de outra fonte. A linha de saída de áudio do Dolby Vision saiu.", .linhas = 9, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_list-video", .nome = "Capítulos por tipo",
    .frase = "Os capítulos no player podem ficar ligados para Filmes, Séries ou Anime, cada um por conta própria. Filmes por padrão.", .linhas = 5 },
  { .icone = "aj_languages", .nome = "Hebraico",
    .frase = "Texto em hebraico tem fonte própria em todo lugar, e legendas ASS em hebraico e árabe ganham uma fonte que as desenha.", .linhas = 5 },
  { .grupo = "Correções", .icone = "aj_captions", .nome = "Legendas",
    .frase = "Cada faixa PGS embutida mantém a própria entrada, e legendas Matroska comprimidas tocam com a conversão.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_monitor-play", .nome = "MP4 com outro áudio",
    .frase = "Um MP4 com áudio de outra fonte lê só a faixa de legenda exibida, então não trava mais.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_link", .nome = "Torrents",
    .frase = "Torrents são resolvidos como no NuvioTV, e um arquivo do TorBox com problema recorre ao que está no painel.", .linhas = 5 },
  { .icone = "aj_fast-forward", .nome = "Créditos",
    .frase = "Num episódio, o último capítulo MKV sem nome só conta como créditos perto do fim, e um nome solto como Ending the War não é mais tomado pelos créditos.", .linhas = 8 },
  { .icone = "aj_tv-minimal-play", .nome = "Botão Trailer",
    .frase = "Com o trailer já tocando ao fundo, o botão Trailer o mantém em vez de começar de novo.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_tag", .nome = "Selos desligados",
    .frase = "O pacote de selos tem Desligado: a lista de fontes não mostra selos, e os cartões do formatador perdem a fileira de selos.", .linhas = 5 },
  { .icone = "aj_funnel", .nome = "Painel de fontes",
    .frase = "Os botões de filtro são ícones em todos os idiomas, e Segure OK: opções da fonte está traduzido em todos.", .linhas = 5 },
  { .icone = "aj_hard-drive", .nome = "Armazenamento da TV",
    .frase = "Fechar o app ou pôr a TV em repouso não deixa mais arquivos de despejo de falha que enchem o armazenamento; os antigos são apagados.", .linhas = 6, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_link", .nome = "Conexões debrid",
    .frase = "Quando um servidor debrid recusa uma conexão, legendas e miniaturas esperam e o deixam para o player, então o TorBox não bloqueia mais a TV por minutos.", .linhas = 6, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_link", .nome = "Arquivos debrid",
    .frase = "Um arquivo do TorBox, AllDebrid ou Real-Debrid que não mostra imagem é conferido, adicionado de novo e reaberto no mesmo ponto antes de tentar a próxima fonte.", .linhas = 7 },
  { .icone = "aj_puzzle", .nome = "Reprodução automática",
    .frase = "Só addons permitidos na reprodução automática seguram a escolha enquanto carregam.", .linhas = 3 },
  { .icone = "aj_captions", .nome = "Abas Áudio e Legendas",
    .frase = "As abas leem só o que mudou, então abrem mais rápido com muitas faixas.", .linhas = 4 },
};
static const NovCena CENAS_2092[] = {
  { "Destaques", "Áudio de outra fonte", visual2092a, 5.5f, 0 },
  { "Destaques", "Sincronia sozinha", visual2092b, 5.5f, 0 },
  { "Destaques", "Trailers do Trailerio", visual2092c, 5.5f, 0 },
  { "Destaques", "Continuar assistindo", visual2092d, 5.5f, 0 },
};
static const int ARTES_2092[] = { 2, 4, 7, 9 };
static const NovConteudo NOV_ATUAL = {
  .versao = "2.0.9.2", .arquivo = "novidades-2092.txt",
  .subtitulo = NULL,
  .itens = ITENS_2092, .nItens = (int)(sizeof ITENS_2092 / sizeof *ITENS_2092),
  .cenas = CENAS_2092, .nCenas = 4,
  .artes = ARTES_2092, .nArtes = 4,
};

// Small, static feature mockups. Only their entrance moves; no live video.
static void visual2092(GfxRect p, float t, float a, int c) {
  float e = ajustes_animacoes_reduzidas() ? 1 : sai3(t / .45f);
  float x = p.x + 40, y = p.y + 100 + (1 - e) * 16;
  float w = p.w - 80;
  const char *icons[] = { "aj_audio-lines", "aj_rotate-cw", "aj_tv-minimal-play", "aj_play" };
  novArte(ARTES_2092[c], (GfxRect){p.x, p.y, p.w, p.h}, PV_RAIO, .68f * a);
  gfx_rect(p, 0, GFX_VEU_CARD, 0, 0, 0, PV_RAIO / p.h, .025f, .03f, .045f, a);
  float vs = fminf(1, (p.h - 230) / 540);
  gfx_transformar(p.x, p.y, vs, 0, 0);
  gfx_icone((GfxRect){x, p.y + 36, 40, 40}, icons[c], .85f, .90f, 1, a);
  if (c == 0) {
    // the picture of one source, the audio of another: two cards and a wave
    novArte(ARTES_2092[0], (GfxRect){x, y, w * .58f, 310}, 18, a * e);
    novArte(ARTES_2092[1], (GfxRect){x + w * .62f, y + 80, w * .38f, 150}, 14, a * e);
    for (int j = 0; j < 12; j++) {
      float h = 10 + (j % 4) * 10;
      gfx_cor((GfxRect){x + w * .62f + j * (w * .38f / 12), y + 290 - h / 2, 6, h}, .4f, .75f, .84f, 1, .8f * a);
    }
  } else if (c == 1) {
    // two timelines lining up
    for (int r = 0; r < 2; r++)
      for (int j = 0; j < 9; j++) {
        float bx = x + j * (w / 9) + (r ? (1 - e) * 40 : 0);
        gfx_cor((GfxRect){bx, y + 60 + r * 90, w / 9 - 14, 36}, .3f, r ? .75f : 1, r ? .84f : 1, 1, (r ? .8f : .35f) * a);
      }
    gfx_icone((GfxRect){x + w / 2 - 28, y + 230, 56, 56}, "aj_captions", 1, 1, 1, a);
  } else if (c == 2) {
    novArte(ARTES_2092[2], (GfxRect){x, y, w, 310}, 20, a * e);
    gfx_icone((GfxRect){x + w / 2 - 36, y + 110, 72, 72}, "aj_play", 1, 1, 1, a);
    gfx_cor((GfxRect){x, y + 340, w, 5}, .5f, 1, 1, 1, .3f * a);
    gfx_cor((GfxRect){x, y + 340, w * .42f, 5}, .5f, .8f, .88f, 1, a);
  } else {
    for (int j = 0; j < 3; j++) {
      float cw = (w - 32) / 3;
      GfxRect r = {x + j * (cw + 16), y + 60, cw, 240};
      novArte(ARTES_2092[j], r, 14, a * e);
      gfx_cor((GfxRect){r.x + 12, r.y + r.h - 18, r.w - 24, 4}, .5f, 1, 1, 1, .3f * a);
      gfx_cor((GfxRect){r.x + 12, r.y + r.h - 18, (r.w - 24) * (.3f + .2f * j), 4}, .5f, .8f, .88f, 1, a);
    }
  }
  gfx_sem_transformar();
}
