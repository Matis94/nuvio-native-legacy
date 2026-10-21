#include "fontesmortas.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

static time_t clockNow = 1000000;
time_t time(time_t *out) { if (out) *out=clockNow; return clockNow; }
char *dados_ler(const char *name) { (void)name; return NULL; }
int dados_gravar(const char *name,const char *value) { (void)name;(void)value;return 1; }
int main(void) {
  Stream *s=calloc(1,sizeof *s);assert(s);
  strcpy(s->provedor,"fixture");strcpy(s->url,"https://fixture.invalid/video?token=one&file=7");
  unsigned v=fontesmortas_versao();assert(fontesmortas_versao()==v);
  fontesmortas_marcar(s);assert(fontesmortas_versao()!=v && fontesmortas_idade(s)==0);
  v=fontesmortas_versao();clockNow+=6*3600-1;
  assert(fontesmortas_versao()==v && fontesmortas_idade(s)>=0);
  clockNow++;assert(fontesmortas_versao()!=v && fontesmortas_idade(s)<0);
  fontesmortas_marcar(s);v=fontesmortas_versao();
  strcpy(s->url,"https://fixture.invalid/video?token=two&file=7");
  assert(fontesmortas_idade(s)>=0);fontesmortas_limpar(s);
  assert(fontesmortas_versao()!=v && fontesmortas_idade(s)<0);
  fontesmortas_marcar(s);v=fontesmortas_versao();fontesmortas_definir_escopo("other",2);
  assert(fontesmortas_versao()!=v && fontesmortas_idade(s)<0);
  free(s);puts("failed-source revision: stable reads, mark/clear, exact expiry and profile isolation OK");
}
