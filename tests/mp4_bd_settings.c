#define NV_WEBOS 1
#include "../src/ajustes.c"
#include <assert.h>
int main(void) {
  assert(AJ_MP4_BD_TEST==AJ_DV7_MAPPING-1 && AJ_MP4_BD_TEST==AJ_RAM_VERDE+1);
  assert(valor[AJ_MP4_BD_TEST]==1 && valorPadrao[AJ_MP4_BD_TEST]==1);
  assert(!strcmp(CHAVE[AJ_MP4_BD_TEST],"mp4BdRemuxTestLocal"));
  assert(ajustes_escopo(AJ_MP4_BD_TEST)==ESC_TV);
  assert(!ajustePortavel(AJ_MP4_BD_TEST) && !ajusteAparelho(AJ_MP4_BD_TEST));
  // Part of "Dolby Vision P7 to P8.1" (owner 27/11): no row of its own, and
  // the MP4 path follows that one switch whatever the old value says.
  assert(foraDestaPlataforma(AJ_MP4_BD_TEST));
  valor[AJ_MP4_BD_TEST]=0;valor[AJ_DV7]=1;assert(!ajustes_mp4_bd_test());
  valor[AJ_MP4_BD_TEST]=1;valor[AJ_DV7]=0;
#if defined(NV_MP4_BD_TEST) && defined(NV_DV7_CONVERSION)
  assert(ajustes_mp4_bd_test());
#else
  assert(!ajustes_mp4_bd_test());
#endif
  puts("PASS MP4 settings: follows Dolby Vision P7 to P8.1, no own row, append-only and device-local");
  return 0;
}
