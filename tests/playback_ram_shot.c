/* Reuse the real GL/font/source-list fixture, then render the RAM overlay
 * above the same video placeholder, with both enabled and disabled states. */
#define main fullnames_fixture_main
#include "fontes_nome_completo_shot.c"
#undef main
#include "ramoverlay.h"

int main(int argc, char **argv) {
  int result = fullnames_fixture_main(argc, argv);
  if (result) return result;
  tecla(SDLK_ESCAPE);
  quadros(45);
  ramoverlay_desenhar(relogio, 1);
  salvar("ram-on");
  quadros(1);
  ramoverlay_desenhar(relogio, 0);
  salvar("ram-off");
  player_abrir(0,NULL);
  noPlayer=1;
  player_shot_video(1);
  VideoSimulacao sim={0};sim.pos=600;sim.duracao=3600;sim.bufferFim=900;
  sim.largura=3840;sim.altura=2160;sim.pronto=1;sim.aceita=1;sim.tocando=1;video_simular(&sim);
  video_simular_download_buffer(32LL*1024*1024,128LL*1024*1024);
  player_shot_estado(relogio,600,3600,1,0,0,0);
  quadros(2);
  int before=ajustes_player_ram();
  SDL_Event green;memset(&green,0,sizeof green);green.type=SDL_KEYDOWN;
  green.key.keysym.scancode=(SDL_Scancode)487;
  player_evento(&green);assert(ajustes_player_ram()!=before);
  green.key.repeat=1;player_evento(&green);assert(ajustes_player_ram()!=before);
  green.key.repeat=0;player_evento(&green);assert(ajustes_player_ram()==before);
  player_shot_estado(relogio,600,3600,1,0,0,0);quadros(3);
  salvar("player-buffer-128");
  printf("Green-button toggle, repeat suppression and buffer OSD rendered\n");
  return 0;
}
