/* New DV7 preference cannot shift the upstream append-only settings arrays. */
#include "../src/ajustes.c"
#include <assert.h>
int main(void) {
  assert(!ajustes_dv7());
  assert(valorPadrao[AJ_DV7]==1 && !strcmp(CHAVE[AJ_DV7],"dv7ConversionLocal"));
  assert(OPCOES[AJ_DV7].tipo==OP_ESCOLHA && OPCOES[AJ_LIMPAR_CACHE].tipo==OP_ACAO);
  assert(!strcmp(CHAVE[AJ_LIMPAR_CACHE],"-limparCache"));
  assert(somenteDesteAparelho(AJ_DV7) && !dePerfil(AJ_DV7));
  assert(!ajustePortavel(AJ_DV7) && !ajusteAparelho(AJ_DV7));
  valor[AJ_DV_MKV]=1; valor[AJ_DV7]=0;
  assert(ajustes_dv7() && !ajustes_dv_mkv());
  assert(!ajustes_aplicar_blob("{\"c_settings_v1\":{\"values\":{\"c_dv7ConversionLocal\":1}}}"));
  assert(ajustes_dv7());
  int n=0;
  for(int i=0;i<AJ_N_TELA;i++) if(TELA[i].tipo==IT_OPC && TELA[i].op==AJ_DV7)n++;
#ifdef NV_DV7_CONVERSION
  assert(n==1);
#else
  assert(n==0);
#endif
  assert(AJ_DTS_RECOVERY==AJ_DV7+1);
  assert(valorPadrao[AJ_DTS_RECOVERY]==0 && !strcmp(CHAVE[AJ_DTS_RECOVERY],"dtsRecoveryLocal"));
  assert(somenteDesteAparelho(AJ_DTS_RECOVERY) && !dePerfil(AJ_DTS_RECOVERY));
  assert(!ajustePortavel(AJ_DTS_RECOVERY) && !ajusteAparelho(AJ_DTS_RECOVERY));
  int recoveryRows=0;
  for(int i=0;i<AJ_N_TELA;i++) if(TELA[i].tipo==IT_OPC && TELA[i].op==AJ_DTS_RECOVERY) recoveryRows++;
#ifdef NV_DTS_WEBOS_RECOVERY
  assert(recoveryRows==1 && ajustes_dts_recovery());
  // All four preference combinations: P7 never changes recovery and vice versa.
  for(int p7=0;p7<2;p7++) for(int recovery=0;recovery<2;recovery++) {
    valor[AJ_DV7]=!p7;valor[AJ_DTS_RECOVERY]=!recovery;
    assert(ajustes_dv7()==p7 && ajustes_dts_recovery()==recovery);
  }
  char dir[]="/tmp/nuvio-independent-dts-settings-XXXXXX";assert(mkdtemp(dir));ajustes_dir(dir);
  valor[AJ_DV7]=1;valor[AJ_DTS_RECOVERY]=0;assert(gravar());
  valor[AJ_DV7]=0;valor[AJ_DTS_RECOVERY]=1;ajustes_dir(dir);
  assert(!ajustes_dv7() && ajustes_dts_recovery());
  valor[AJ_DV7]=0;valor[AJ_DTS_RECOVERY]=1;assert(gravar());
  valor[AJ_DV7]=1;valor[AJ_DTS_RECOVERY]=0;ajustes_dir(dir);
  assert(ajustes_dv7() && !ajustes_dts_recovery());
  assert(!ajustes_aplicar_blob("{\"dtsRecoveryLocal\":0}"));
  assert(!ajustes_aplicar_blob("{\"c_settings_v1\":{\"values\":{\"c_dtsRecoveryLocal\":0}}}"));
  assert(!ajustes_dts_recovery());
  char *blob=NULL;ajustes_mesclar_blob("{}",&blob);assert(!blob || !strstr(blob,"dtsRecoveryLocal"));free(blob);
  char path[512];snprintf(path,sizeof path,"%s/ajustes.txt",dir);unlink(path);rmdir(dir);
#else
  assert(!recoveryRows && !ajustes_dts_recovery());
  valor[AJ_DTS_RECOVERY]=0;assert(!ajustes_dts_recovery());
#endif
  puts("PASS: independent DV7/recovery preferences, local persistence, sync isolation and compile-time boundaries");
}
