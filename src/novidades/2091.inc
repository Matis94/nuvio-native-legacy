// The 2.0.9.1 release card: what this build adds over 2.0.8. English only for
// now. Keep it up to date with every change of the build.
static void visual2091(GfxRect p, float t, float a, int c);
static void visual2091a(GfxRect p, float t, float a) { visual2091(p, t, a, 0); }
static void visual2091b(GfxRect p, float t, float a) { visual2091(p, t, a, 1); }
static void visual2091c(GfxRect p, float t, float a) { visual2091(p, t, a, 2); }
static void visual2091d(GfxRect p, float t, float a) { visual2091(p, t, a, 3); }
static const NovItem ITENS_2091[] = {
  { .grupo = "Highlights", .icone = "aj_audio-lines", .nome = "Audio from another source",
    .frase = "Keep the picture of one source and play the audio of another version of the same title, such as a dub. One page lists every version by language, and the next episodes keep the same version and track.", .linhas = 10, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_rotate-cw", .nome = "Sync by itself",
    .frase = "The subtitles place the other version at once, then its audio checks it. The sync is checked all through the episode and corrected at quiet moments. Audio sync, Scan again runs it once more and lets you pick the track to compare.", .linhas = 11, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_monitor-play", .nome = "HLS (experimental)",
    .frase = "Plays the DTS and Dolby Vision P7 conversions over HLS. The TV picks fMP4 or MPEG-TS by itself.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_tv-minimal-play", .nome = "Trailerio trailers",
    .frase = "Automatic trailers try Trailerio after Apple TV and IMDb, and Left and Right seek 5 s.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_play", .nome = "Continue watching",
    .frase = "New episode and New season badges, newly aired episodes move to the front, and resume opens the episode at once with its source and its audio.", .linhas = 7 },
  { .grupo = "More", .icone = "aj_headphones", .nome = "DTS-HD in the audio menu",
    .frase = "DTS-HD and DTS-HD MA are told apart from plain DTS, with DTS off in Settings too.", .linhas = 4, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_headphones", .nome = "DTS from another source",
    .frase = "With DTS conversion off, a TV that plays DTS gets the other version's DTS tracks as they are.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_puzzle", .nome = "Source icons",
    .frase = "Sources From addon draw emoji, flags and symbols as icons, in a Standard or Minimalistic style. A star rating shows five matching stars.", .linhas = 7 },
  { .icone = "aj_list-video", .nome = "Chapters and skips",
    .frase = "MKV chapters are read like NuvioTV, with intro, recap, credits and post-credits skips from IntroDB, Anime-Skip and AniSkip. Picking a chapter while scrubbing seeks once.", .linhas = 8 },
  { .icone = "aj_search", .nome = "Search cursor",
    .frase = "The text cursor in Search, Spotlight and the keyboard is easier to see.", .linhas = 4 },
  { .icone = "aj_download", .nome = "P2P engine 0.1.7",
    .frase = "The built-in P2P uses the engine NuvioTV ships: torrents start faster and stall less on slow peers.", .linhas = 5 },
  { .icone = "aj_rotate-ccw-clock", .nome = "Back after Home or sleep",
    .frase = "Leave with Home or let the TV sleep: back in the app, the movie reopens paused at the exact point, and DTS and Dolby Vision titles go back on their own path.", .linhas = 8, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_film", .nome = "Movies in parts",
    .frase = "In the credits of a part, a countdown to Part N of M with Start now or Keep watching. A next-part link that waited too long is fetched again.", .linhas = 7 },
  { .icone = "aj_film", .nome = "Seek-bar thumbnails",
    .frase = "The TV makes the seek-bar thumbnails itself from the file while it plays: no key, any title. Up on the bar opens the chapter row with a picture for every chapter.", .linhas = 8, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_monitor-play", .nome = "Dolby Vision P7 to P8.1",
    .frase = "One switch converts both MKV and MP4 Dolby Vision profile 7, no longer marked experimental. Off, a Dolby Vision MP4 the TV refuses still plays, in HDR10. The Dolby Vision audio output row is gone.", .linhas = 9, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_list-video", .nome = "Chapters per kind",
    .frase = "Chapters in the player can be on for Movies, TV Shows or Anime, each on its own. Movies by default.", .linhas = 5 },
  { .icone = "aj_languages", .nome = "Hebrew",
    .frase = "Hebrew text has its own font everywhere, and Hebrew and Arabic ASS subtitles get a face that draws them.", .linhas = 5 },
  { .grupo = "Fixes", .icone = "aj_captions", .nome = "Subtitles",
    .frase = "Every embedded PGS track keeps its own entry, and compressed Matroska subtitles play with the conversion.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_monitor-play", .nome = "MP4 with another audio",
    .frase = "An MP4 with audio from another source reads only the subtitle track shown, so it no longer stalls.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_link", .nome = "Torrents",
    .frase = "Torrents resolve like NuvioTV, and a TorBox file with an issue falls back to the one on the dashboard.", .linhas = 5 },
  { .icone = "aj_fast-forward", .nome = "Credits",
    .frase = "In an episode, the last unnamed MKV chapter counts as the credits only near the end, and a loose chapter name like Ending the War is no longer taken for the credits.", .linhas = 8 },
  { .icone = "aj_tv-minimal-play", .nome = "Trailer button",
    .frase = "With the trailer already playing in the background, the Trailer button keeps it instead of starting it again.", .linhas = 5, .plataformas = NOV_LG | NOV_OUTRAS },
  { .icone = "aj_tag", .nome = "Badges off",
    .frase = "The badge pack choice has Off: the source list shows no badges, and the formatter cards lose their badge row.", .linhas = 5 },
  { .icone = "aj_funnel", .nome = "Sources sheet",
    .frase = "The filter buttons are icons in every language, and Hold OK: source options is translated everywhere.", .linhas = 5 },
  { .icone = "aj_link", .nome = "Debrid files",
    .frase = "A TorBox, AllDebrid or Real-Debrid file that shows no picture is checked, added again and reopened at the same point before the next source is tried.", .linhas = 7 },
  { .icone = "aj_puzzle", .nome = "Autoplay",
    .frase = "Only add-ons allowed for autoplay hold the pick while they load.", .linhas = 3 },
  { .icone = "aj_captions", .nome = "Audio and Subtitles tabs",
    .frase = "The tabs read only what changed, so they open faster with many tracks.", .linhas = 4 },
};
static const NovCena CENAS_2091[] = {
  { "Highlights", "Audio from another source", visual2091a, 5.5f, 0 },
  { "Highlights", "Sync by itself", visual2091b, 5.5f, 0 },
  { "Highlights", "Trailerio trailers", visual2091c, 5.5f, 0 },
  { "Highlights", "Continue watching", visual2091d, 5.5f, 0 },
};
static const int ARTES_2091[] = { 2, 4, 7, 9 };
static const NovConteudo NOV_ATUAL = {
  .versao = "2.0.9.1", .arquivo = "novidades-2091.txt",
  .subtitulo = NULL,
  .itens = ITENS_2091, .nItens = (int)(sizeof ITENS_2091 / sizeof *ITENS_2091),
  .cenas = CENAS_2091, .nCenas = 4,
  .artes = ARTES_2091, .nArtes = 4,
};

// Small, static feature mockups. Only their entrance moves; no live video.
static void visual2091(GfxRect p, float t, float a, int c) {
  float e = ajustes_animacoes_reduzidas() ? 1 : sai3(t / .45f);
  float x = p.x + 40, y = p.y + 100 + (1 - e) * 16;
  float w = p.w - 80;
  const char *icons[] = { "aj_audio-lines", "aj_rotate-cw", "aj_tv-minimal-play", "aj_play" };
  novArte(ARTES_2091[c], (GfxRect){p.x, p.y, p.w, p.h}, PV_RAIO, .68f * a);
  gfx_rect(p, 0, GFX_VEU_CARD, 0, 0, 0, PV_RAIO / p.h, .025f, .03f, .045f, a);
  float vs = fminf(1, (p.h - 230) / 540);
  gfx_transformar(p.x, p.y, vs, 0, 0);
  gfx_icone((GfxRect){x, p.y + 36, 40, 40}, icons[c], .85f, .90f, 1, a);
  if (c == 0) {
    // the picture of one source, the audio of another: two cards and a wave
    novArte(ARTES_2091[0], (GfxRect){x, y, w * .58f, 310}, 18, a * e);
    novArte(ARTES_2091[1], (GfxRect){x + w * .62f, y + 80, w * .38f, 150}, 14, a * e);
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
    novArte(ARTES_2091[2], (GfxRect){x, y, w, 310}, 20, a * e);
    gfx_icone((GfxRect){x + w / 2 - 36, y + 110, 72, 72}, "aj_play", 1, 1, 1, a);
    gfx_cor((GfxRect){x, y + 340, w, 5}, .5f, 1, 1, 1, .3f * a);
    gfx_cor((GfxRect){x, y + 340, w * .42f, 5}, .5f, .8f, .88f, 1, a);
  } else {
    for (int j = 0; j < 3; j++) {
      float cw = (w - 32) / 3;
      GfxRect r = {x + j * (cw + 16), y + 60, cw, 240};
      novArte(ARTES_2091[j], r, 14, a * e);
      gfx_cor((GfxRect){r.x + 12, r.y + r.h - 18, r.w - 24, 4}, .5f, 1, 1, 1, .3f * a);
      gfx_cor((GfxRect){r.x + 12, r.y + r.h - 18, (r.w - 24) * (.3f + .2f * j), 4}, .5f, .8f, .88f, 1, a);
    }
  }
  gfx_sem_transformar();
}
