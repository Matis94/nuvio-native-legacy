#include "../src/ajustes.c"
#include <assert.h>

int main(void) {
  char dir[] = "/tmp/nuvio-ram-settings-XXXXXX";
  char *blob = NULL;
  assert(!ajustes_player_ram());
  assert(ajustes_movie_buffer_mib()==64 && ajustes_dv7());
  for(int i=0;i<4;i++) {
    const int expected[]={64,128,256,0};valor[AJ_MOVIE_BUFFER]=i;
    assert(ajustes_movie_buffer_mib()==expected[i]);
  }
  valor[AJ_MOVIE_BUFFER]=0;
  valor[AJ_DV_MKV]=1; assert(!ajustes_dv_mkv() && ajustes_dv7());
  valor[AJ_DV7]=1;valor[AJ_DV_MKV]=0;assert(!ajustes_dv7() && ajustes_dv_mkv());
  valor[AJ_DV7]=0;
  assert(somenteDesteAparelho(AJ_DV7) && somenteDesteAparelho(AJ_MOVIE_BUFFER));
  assert(valorPadrao[AJ_PLAYER_RAM] == 1);
  assert(somenteDesteAparelho(AJ_PLAYER_RAM) && !dePerfil(AJ_PLAYER_RAM));
  assert(!strcmp(CHAVE[AJ_PLAYER_RAM], "playerRamOverlayLocal"));
  int occurrences = 0;
  for (int i = 0; i < AJ_N_TELA; i++)
    if (TELA[i].tipo == IT_OPC && TELA[i].op == AJ_PLAYER_RAM) occurrences++;
  assert(occurrences == 1);
  assert(mkdtemp(dir));
  ajustes_dir(dir);
  assert(!ajustes_player_ram());
  valor[AJ_PLAYER_RAM] = 0;
  assert(gravar());
  valor[AJ_PLAYER_RAM] = 1;
  ajustes_dir(dir);
  assert(ajustes_player_ram());
  assert(!ajustes_aplicar_blob("{\"playerRamOverlayLocal\":1}"));
  assert(ajustes_player_ram());
  ajustes_mesclar_blob("{}", &blob);
  assert(!blob || !strstr(blob, "playerRamOverlayLocal"));
  free(blob);
  valor[AJ_PLAYER_RAM] = 1;
  assert(gravar());
  valor[AJ_PLAYER_RAM] = 0;
  ajustes_dir(dir);
  assert(!ajustes_player_ram());
  puts("Playback RAM settings: default off, local-only, one row and persisted on/off passed");
  return 0;
}
