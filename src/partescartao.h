// CARTAO DAS PARTES, sobre o video do player (2.0.9 do Namer, 97f22a3f2):
//   CONTAGEM: filme em partes, nos creditos de uma parte que nao e a ultima.
//   "Parte N de M" em N s, Comecar agora / Continuar assistindo. Voltar ou
//   Continuar = fica na parte (ela ainda troca sozinha no fim).
// Quem decide QUANDO abrir e quem atende o pedido e app.c (vigiarPartes); o
// player desenha e repassa as teclas.
#ifndef NV_PARTESCARTAO_H
#define NV_PARTESCARTAO_H
#include <SDL2/SDL.h>

#define PC_PEDIDO_NENHUM (-1)
#define PC_PEDIDO_AGORA  (-2)   // contagem: tocar a parte seguinte ja
#define PC_PEDIDO_FICAR  (-3)   // contagem: continuar nesta parte

void partescartao_contagem(int parteN, int total, Uint32 agora, int contagemSeg);
void partescartao_fechar(void);
void partescartao_zerar(void);
int  partescartao_visivel(void);
int  partescartao_modo_contagem(void);
// Pedido consumido uma vez: PC_PEDIDO_*.
int  partescartao_pedido(void);
void partescartao_atualizar(float dt, Uint32 agora);
// 0 nao consumiu; 1 consumiu; 2 consumiu E o player mostra os controles.
int  partescartao_evento(const SDL_Event *e);
void partescartao_desenhar(Uint32 agora);
#endif
