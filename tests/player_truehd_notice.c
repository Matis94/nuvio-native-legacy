#include "player_dts_notice.h"
#include "plrilha.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define PG_SEG_TOTAL 6.0f
static NvDtsNotice truehdConvertedNotice;
static Uint32 truehdDesde,pgDesde;
static int truehdVisto,loading,channel,converted,playing=1,starting,calls;
static char message[256];
int player_carregando(void) { return loading; }
static int ehCanal(void) { return channel; }
int video_truehd_convertido(void) { return converted; }
int video_tocando(void) { return playing; }
int video_iniciando(void) { return starting; }
const char *i18n(const char *s) { (void)s;return "Dźwięk DTS jest konwertowany na tym TV"; }
void plrilha_pedir(const PlrIlhaPedido *p) {
  assert(!strcmp(p->icone,"aj_audio-lines") && p->semFim && p->aberta);
  snprintf(message,sizeof message,"%s",p->texto);calls++;
}
#include "truehd-render.c"
int main(void) {
  converted=1;loading=1;avisarTruehd(100);avisarTruehd(3000);assert(!calls);
  loading=0;starting=1;avisarTruehd(4000);assert(!calls);
  starting=0;avisarTruehd(5000);avisarTruehd(6499);assert(!calls);
  avisarTruehd(6500);assert(calls==1);
  assert(!strcmp(message,"Dźwięk TrueHD jest konwertowany na tym TV"));
  avisarTruehd(11499);assert(calls==2);avisarTruehd(11500);assert(calls==2);
  for(int i=0;i<5;i++) {
    loading=1;playing=0;avisarTruehd(12000+i*10000);
    loading=0;playing=1;avisarTruehd(14000+i*10000);avisarTruehd(16000+i*10000);
    converted=0;avisarTruehd(17000+i*10000);converted=1;
  }
  assert(calls==2); // Neither seeks, reloads nor switching away/back repeats it.
  truehdConvertedNotice=(NvDtsNotice){0};truehdVisto=0;truehdDesde=0;
  channel=1;avisarTruehd(70000);channel=0;pgDesde=71000;
  avisarTruehd(72000);avisarTruehd(76000);assert(calls==2);
  pgDesde=0;avisarTruehd(77499);assert(calls==2);
  avisarTruehd(77500);assert(calls==3); // A new source may announce after the parental card.
  puts("TrueHD notice: localized actual renderer, selected conversion only, startup/bind/parental gates and once per source across seeks/reloads passed");
}
