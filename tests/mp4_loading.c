/* Execute the production text-slot renderer. No native player or GL context. */
#include "video.h"
#include "dvtela.h"
#include "text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static VideoDvFase phase;
static DvtelaEstado state;
static float alpha;
static int calls;
static char line[160];
void video_dv_fase(VideoDvFase *out) { *out=phase; }
const DvtelaEstado *dvtela_estado(void) { return &state; }
float dvtela_alfa(void) { return alpha; }
const char *i18n(const char *s) { return s; }
TxtLinha txt_linha(TxtEstilo style,const char *s,int r,int g,int b,int a) {
  (void)style;(void)r;(void)g;(void)b;(void)a;
  snprintf(line,sizeof line,"%s",s);calls++;return (TxtLinha){.w=100,.h=24};
}
void txt_desenhar_alpha(TxtLinha l,float x,float y,float a) { (void)l;(void)x;(void)y;assert(a>0); }
#define ZONA(z,r) ((void)0)
#include "mp4-loading-render.c"
static void expect(const char *text,int corner) {
  calls=0;line[0]=0;float height=dvtela_abertura_linha(corner,960,800,1);
  if(text) {assert(calls==1 && !strcmp(line,text));assert(height==34);}
  else assert(calls==0 && height==0);
}
int main(void) {
  for(int corner=0;corner<=1;corner++) {
    phase=(VideoDvFase){0};state=(DvtelaEstado){0};alpha=0;
    expect(NULL,corner); // Healthy native MP4 and ordinary SDR opening.
    phase.mp4Inicial=1;expect("Lendo o arquivo",corner);
    phase.caminho=1;expect("Abrindo a fonte",corner);
    phase.carregado=1;expect("Preparando a imagem",corner);
    phase.mp4Inicial=0;expect(NULL,corner); // First picture, failure, seek/audio reload or new native source.
    alpha=1;state.ativa=1;state.passo=DVT_PASSO_IMAGEM;
    expect("Preparando a imagem",corner); // Existing MKV screen unchanged.
    state.passo=DVT_PASSO_ABRIR;expect("Abrindo a fonte",corner);
  }
  puts("PASS MP4 startup text: real read/open/image stages, both layouts, no native-source or post-startup text, existing MKV stages retained");
  return 0;
}
