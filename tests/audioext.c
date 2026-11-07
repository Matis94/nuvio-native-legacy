#define _POSIX_C_SOURCE 200809L
// The audio-source state (audioext.c): marks per title, menu choice requests,
// offset, and the series memory that carries a choice to the next episodes.
#include "../src/audioext.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static int fakeSonda(const char *url, const char *cab, AudioExtFaixa *out, int max) {
  (void)cab; if (strstr(url, "bad")) return -1;
  for (int i = 0; i < 2 && i < max; i++) { memset(&out[i], 0, sizeof out[i]); out[i].stream = i + 1; snprintf(out[i].idioma, sizeof out[i].idioma, "%s", i ? "eng" : "pol"); }
  return 2;
}
static int esperarSonda(unsigned g, AudioExtFaixa *f) {
  int n = -2; struct timespec ts = {0, 2000000};
  for (int i = 0; i < 1000 && (n = audioext_sonda_resultado(g, f, AUDIOEXT_FAIXAS_MAX)) == -2; i++) nanosleep(&ts, NULL);
  return n;
}

int main(void) {
  AudioExtIdent id, got;
  audioext_preferencias(0, 1);
  audioext_alvo("tt1:1:1");
  unsigned g = audioext_pedido_geracao();
  audioext_escolher("https://a/pl.mkv", "X: 1", "WEB-DL PL", "pl");
  assert(audioext_pedido_geracao() == g + 1 && audioext_pedido("https://a/remux.mkv"));
  assert(!audioext_pedido("https://a/pl.mkv"));          // the picture's own file is not "another"
  memset(&id, 0, sizeof id);
  snprintf(id.grupo, sizeof id.grupo, "aio|webdl-pl"); snprintf(id.idioma, sizeof id.idioma, "pl");
  audioext_lembrar_serie(&id);
  audioext_offset_somar(150);
  // The same episode never looks for itself.
  assert(!audioext_serie_procura("tt1:1:1", NULL));
  // Raw title id stamped before the episode list: same title, mark kept.
  audioext_alvo("tt1");
  assert(audioext_tem_marca());
  // Next episode: the mark goes, the memory stays and is looked for.
  audioext_alvo("tt1:1:2");
  assert(!audioext_tem_marca());
  assert(audioext_serie_procura("tt1:1:2", &got) && !strcmp(got.grupo, "aio|webdl-pl"));
  g = audioext_pedido_geracao();
  audioext_serie_achou("https://a/pl-e2.mkv", "X: 1", "WEB-DL PL E2");
  assert(audioext_pedido_geracao() == g + 1);
  assert(audioext_offset_ms() == 150 && !strcmp(audioext_idioma(), "pl"));
  assert(audioext_pedido("https://a/remux-e2.mkv"));
  assert(!audioext_serie_procura("tt1:1:2", NULL));      // already chosen here
  // A new offset in episode 2 is what episode 3 gets.
  audioext_offset_somar(-50);
  audioext_alvo("tt1:1:3");
  assert(audioext_serie_procura("tt1:1:3", NULL));
  audioext_serie_achou("https://a/pl-e3.mkv", "", "E3");
  assert(audioext_offset_ms() == 100);
  // Another series does not inherit it.
  audioext_alvo("tt2:1:1");
  assert(!audioext_serie_procura("tt2:1:1", NULL));
  // Back to the series, then "This file's own audio": memory forgotten.
  audioext_alvo("tt1:1:4");
  assert(audioext_serie_procura("tt1:1:4", NULL));
  audioext_original();
  assert(!audioext_serie_procura("tt1:1:4", NULL));
  // Setting off: nothing remembered.
  audioext_alvo("tt3:1:1");
  audioext_escolher("https://b/pl.mkv", "", "PL", "pl");
  audioext_preferencias(1, 0);
  audioext_lembrar_serie(&id);
  audioext_alvo("tt3:1:2");
  assert(!audioext_serie_procura("tt3:1:2", NULL) && audioext_so_audio());
  // Movies (no episode) are never remembered as a series.
  audioext_preferencias(0, 1);
  audioext_alvo("tt9");
  audioext_escolher("https://c/pl.mkv", "", "PL", "pl");
  audioext_lembrar_serie(&id);
  assert(!audioext_serie_procura("tt9", NULL));
  // SAFETY NET: a start whose audio never played makes the next start of the
  // title use the file's own audio.
  audioext_alvo("tt20");
  assert(audioext_alternar("https://d/pl.mkv", "", "PL") == 1);
  audioext_inicio_reproducao("https://d/remux.mkv");          // first start: tries it
  assert(audioext_pedido("https://d/remux.mkv"));
  audioext_inicio_reproducao("https://d/remux.mkv");          // backed out while loading
  assert(!audioext_tem_marca() && audioext_voltou_padrao_consumir());
  assert(!audioext_voltou_padrao_consumir());
  // A start whose audio played keeps the mark for the next one.
  assert(audioext_alternar("https://d/pl.mkv", "", "PL") == 1);
  audioext_inicio_reproducao("https://d/remux.mkv");
  audioext_tocando_definir(1);
  audioext_tocando_definir(0);
  audioext_inicio_reproducao("https://d/remux.mkv");
  assert(audioext_pedido("https://d/remux.mkv") && !audioext_voltou_padrao_consumir());
  // Playing the marked source itself is not an attempt.
  audioext_tocando_definir(1); audioext_tocando_definir(0);
  audioext_inicio_reproducao("https://d/pl.mkv");
  audioext_inicio_reproducao("https://d/pl.mkv");
  assert(audioext_tem_marca());
  // A reported failure clears the mark at once.
  audioext_falhou("no audio");
  assert(!audioext_tem_marca());
  { char f[32]; assert(audioext_falha_consumir(f, sizeof f) && !strcmp(f, "no audio")); }
  // A track of the other version picked in the menu: reopens with it, and is
  // forgotten with the mark or a new version.
  audioext_alvo("tt21");
  assert(audioext_alternar("https://e/pl.mkv", "", "PL") == 1 && audioext_faixa() == -1);
  { unsigned g = audioext_pedido_geracao();
    audioext_escolher_faixa(2);
    assert(audioext_faixa() == 2 && audioext_pedido_geracao() == g + 1); }
  audioext_escolher("https://e/pl.mkv", "", "PL", "pl");   // same version: the track stays
  assert(audioext_faixa() == 2);
  audioext_escolher("https://e/other.mkv", "", "PL2", "pl");
  assert(audioext_faixa() == -1);
  audioext_escolher_faixa(1); audioext_limpar();
  assert(audioext_faixa() == -1);
  audioext_escolher_faixa(1);   // no mark: nothing to pick from
  assert(audioext_faixa() == -1);
  // Tracks of a version for the hold-OK menu: read on a thread, by request.
  { AudioExtFaixa f[AUDIOEXT_FAIXAS_MAX];
    assert(!audioext_sonda_disponivel());
    assert(esperarSonda(audioext_sondar("https://x/a.mkv", ""), f) == -1);   // no reader
    audioext_definir_sonda(fakeSonda);
    unsigned g = audioext_sondar("https://x/a.mkv", "");
    assert(esperarSonda(g, f) == 2 && f[0].stream == 1 && !strcmp(f[1].idioma, "eng"));
    assert(esperarSonda(audioext_sondar("https://x/bad.mkv", ""), f) == -1);
    assert(audioext_sonda_resultado(g, f, AUDIOEXT_FAIXAS_MAX) == -1); }  // an older request
  // The track picked before playing: set, nothing reopened, nothing pending.
  audioext_alvo("tt22");
  assert(audioext_alternar("https://e/v.mkv", "", "V") == 1);
  { unsigned g = audioext_pedido_geracao();
    audioext_definir_faixa(3);
    assert(audioext_faixa() == 3 && audioext_pedido_geracao() == g);
    audioext_inicio_reproducao("https://e/main.mkv");
    assert(audioext_tem_marca() && !audioext_voltou_padrao_consumir()); }
  puts("audioext: marks, menu requests, offsets and series memory passed");
  return 0;
}
