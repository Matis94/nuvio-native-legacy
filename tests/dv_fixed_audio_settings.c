/* Real settings table/getter: the experimental choice must never silently
 * enable on upgrade, move existing preferences or propagate via the account. */
#include "ajustes.h"
#define NV_WEBOS 1
#include "../src/ajustes.c"
#include <assert.h>
int main(void) {
  assert(AJ_DV_AUDIO_FIXO==AJ_MP4_BD_TEST-4 && valor[AJ_DV_AUDIO_FIXO]==0);
  assert(!strcmp(CHAVE[AJ_DV_AUDIO_FIXO],"dvFixedAudioTestLocal"));
  assert(ajustes_escopo(AJ_DV_AUDIO_FIXO)==ESC_TV);
  assert(!ajustePortavel(AJ_DV_AUDIO_FIXO) && !ajusteAparelho(AJ_DV_AUDIO_FIXO));
  assert(!ajustes_dv_audio_fixo());
  // Removed (owner 27/11): hidden everywhere and always the original audio.
  for(int mode=1;mode<=2;mode++) {
    valor[AJ_DV_AUDIO_FIXO]=mode;
    assert(!ajustes_dv_audio_fixo() && foraDestaPlataforma(AJ_DV_AUDIO_FIXO));
  }
  valor[AJ_DV_AUDIO_FIXO]=77;assert(!ajustes_dv_audio_fixo());
  assert(!strcmp(OPCOES[AJ_DV_AUDIO_FIXO].rotulo,"Dolby Vision audio output"));
  puts("Fixed DV audio settings: removed row, always original audio, append-only ID and device-only scope passed");
  return 0;
}
