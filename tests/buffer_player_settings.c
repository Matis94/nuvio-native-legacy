#include "ajustes.h"
#define NV_WEBOS 1
#include "../src/ajustes.c"
#include <assert.h>
#include <unistd.h>
// Platform notification boundary; the host fixture does not query a TV.
void avisos_tv_linha(char *line,size_t cap,char *model,size_t model_cap) {
  if(cap)line[0]=0;if(model_cap)model[0]=0;
}
static void key(SDL_Keycode k) { assert(uxEvento(k)); }
static void open_buffer(void) { uxAbrirEditor(AJ_BUFFER);assert(uxEditor==1); }
static void switch_row(void) {
  for(int i=uxPendente;i<nValores(AJ_BUFFER)-1;i++)key(SDLK_DOWN);
  key(SDLK_DOWN);assert(uxBufferDadosFoco);
}
int main(void) {
  char dir[]="/tmp/nuvio-buffer-settings-XXXXXX";assert(mkdtemp(dir));
  assert(SDL_Init(SDL_INIT_TIMER)==0);setenv("NUVIO_DADOS",dir,1);
  dados_iniciar(dir);snprintf(dirAjustes,sizeof dirAjustes,"%s",dir);ajustes_iniciar();
  assert(gravar());assert(!ajustes_buffer_dados());
  assert(AJ_BUFFER_DADOS==AJ_N-1 && ajustes_escopo(AJ_BUFFER_DADOS)==ESC_TV);
  assert(!ajustePortavel(AJ_BUFFER_DADOS) && !ajusteAparelho(AJ_BUFFER_DADOS));
  int before=ajustes_buffer_preset();
  open_buffer();switch_row();key(SDLK_RETURN);
  assert(uxPendente==before && uxBufferDados==0 && !ajustes_buffer_dados());
  key(SDLK_ESCAPE);assert(!uxEditor && !ajustes_buffer_dados() && ajustes_buffer_preset()==before);
  // Save the toggle with an unchanged size: this must still write the file.
  open_buffer();switch_row();key(SDLK_RETURN);
  assert(uxPendente==before && uxBufferDadosFoco && uxBufferDados==0);
  key(SDLK_DOWN);assert(uxRodape==2);key(SDLK_RETURN);
  assert(!uxEditor && ajustes_buffer_dados() && ajustes_buffer_preset()==before);
  valor[AJ_BUFFER_DADOS]=1;ajustes_dir(dir);assert(ajustes_buffer_dados());
  // Mouse choice clears toggle focus; Cancel preserves the committed state.
  open_buffer();ajPontBufferAlternar(0,0);ajPontEdValor(2,0);
  assert(!uxBufferDadosFoco && uxPendente==2);
  switch_row();key(SDLK_DOWN);key(SDLK_RIGHT);assert(uxRodape==3);key(SDLK_RETURN);
  assert(!uxEditor && ajustes_buffer_dados() && ajustes_buffer_preset()==before);
  // One confirmation restores both the size and the diagnostic default.
  open_buffer();uxRestaurar=1;uxAplicar();
  assert(!uxEditor && !ajustes_buffer_dados() && ajustes_buffer_preset()==0);
  // Fixed audio preference uses the actual UI save/reload path, not just a getter.
  uxAbrirEditor(AJ_DV_AUDIO_FIXO);assert(uxEditor==1);
  uxPendente=1;uxAplicar();assert(ajustes_dv_audio_fixo()==1);
  valor[AJ_DV_AUDIO_FIXO]=0;ajustes_dir(dir);assert(ajustes_dv_audio_fixo()==1);
  // A failed combined save cannot partially change the live preferences.
  open_buffer();uxPendente=3;uxBufferDados=0;
  char saved[sizeof dirAjustes];memcpy(saved,dirAjustes,sizeof saved);
  snprintf(dirAjustes,sizeof dirAjustes,"/dev/null/not-a-directory");uxAplicar();
  assert(uxEditor && !ajustes_buffer_dados() && ajustes_buffer_preset()==0);
  memcpy(dirAjustes,saved,sizeof saved);uxCancelar();
  SDL_Quit();
  puts("PASS buffer data chooser: default off, remote/mouse, draft/cancel, atomic save/reload/restore, write failure and fixed DV audio preference persistence");
  return 0;
}
