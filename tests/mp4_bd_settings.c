#define NV_WEBOS 1
#include "../src/ajustes.c"
#include <assert.h>
int main(void) {
  assert(AJ_MP4_BD_TEST==AJ_DV7_MAPPING-1 && AJ_MP4_BD_TEST==AJ_RAM_VERDE+1);
  assert(valor[AJ_MP4_BD_TEST]==1 && valorPadrao[AJ_MP4_BD_TEST]==1);
  assert(!strcmp(CHAVE[AJ_MP4_BD_TEST],"mp4BdRemuxTestLocal"));
  assert(ajustes_escopo(AJ_MP4_BD_TEST)==ESC_TV);
  assert(!ajustePortavel(AJ_MP4_BD_TEST) && !ajusteAparelho(AJ_MP4_BD_TEST));
  // No row of its own (owner 27/11): the MP4 recovery is always on with the
  // converter, and "Dolby Vision P7 to P8.1" only picks P8.1 or HDR10.
  assert(foraDestaPlataforma(AJ_MP4_BD_TEST));
  for(int dv7=0;dv7<=1;dv7++) for(int old=0;old<=1;old++) {
    valor[AJ_DV7]=dv7;valor[AJ_MP4_BD_TEST]=old;
#if defined(NV_MP4_BD_TEST) && defined(NV_DV7_CONVERSION)
    assert(ajustes_mp4_bd_test());
#else
    assert(!ajustes_mp4_bd_test());
#endif
  }
  puts("PASS MP4 settings: recovery always on, no own row, append-only and device-local");
  return 0;
}
