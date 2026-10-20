#include "../src/ajustes.c"
#include <assert.h>

int main(void) {
  char dir[] = "/tmp/nuvio-ram-settings-XXXXXX";
  char *blob = NULL;
  assert(!ajustes_player_ram());
  assert(ajustes_buffer_preset()==0 && ajustes_dv7());
  valor[AJ_DV_MKV]=1; assert(!ajustes_dv_mkv() && ajustes_dv7());
  valor[AJ_DV7]=1;valor[AJ_DV_MKV]=0;assert(!ajustes_dv7() && ajustes_dv_mkv());
  valor[AJ_DV7]=0;
  assert(somenteDesteAparelho(AJ_DV7) && somenteDesteAparelho(AJ_BUFFER));
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
  assert(!ajustePortavel(AJ_DV7) && !ajusteAparelho(AJ_DV7));
  assert(!ajustePortavel(AJ_PLAYER_RAM) && !ajusteAparelho(AJ_PLAYER_RAM));
  assert(!ajustes_aplicar_blob("{\"c_settings_v1\":{\"values\":{\"c_playerRamOverlayLocal\":1}}}"));
  assert(ajustes_player_ram());
  free(blob);
  valor[AJ_PLAYER_RAM] = 1;
  assert(gravar());
  valor[AJ_PLAYER_RAM] = 0;
  ajustes_dir(dir);
  assert(!ajustes_player_ram());
  // The legacy custom setting is consumed once and no longer serialized.
  char path[512]; snprintf(path,sizeof path,"%s/ajustes.txt",dir);
  for(int old=0;old<4;old++) {
    FILE *f=fopen(path,"w"); assert(f);
    fprintf(f,"bufferPlayerLocal 0\nmovieDownloadBufferLocal %d\n",old); fclose(f);
    ajustes_dir(dir);
    const int expected[]={1,2,3,0};
    assert(ajustes_buffer_preset()==expected[old]);
    assert(gravar());
    f=fopen(path,"r"); assert(f); char line[256];
    while(fgets(line,sizeof line,f)) assert(!strstr(line,"movieDownloadBufferLocal"));
    fclose(f);
    valor[AJ_BUFFER]=0; ajustes_dir(dir);
    assert(ajustes_buffer_preset()==expected[old]);
  }
  puts("Playback RAM settings, official buffer and legacy migration passed");
  return 0;
}
