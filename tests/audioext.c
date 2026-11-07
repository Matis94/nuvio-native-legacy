// The audio-source state (audioext.c): marks per title, menu choice requests,
// offset, and the series memory that carries a choice to the next episodes.
#include "../src/audioext.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

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
  puts("audioext: marks, menu requests, offsets and series memory passed");
  return 0;
}
