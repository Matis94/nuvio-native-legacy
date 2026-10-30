// A long formatter description must not push the supplied filename past the
// bounded source-line window. Use the real parser and text normalization.
#include "../src/streams.c"
#include <assert.h>
int main(void) {
  static Stream s;
  static LinhaAddon body[FOLHA_ADDON_LINHAS];
  memset(s.arquivo,'A',850);strcpy(s.arquivo+850,".Full.Filename.End.mkv");
  for(int i=0;i<14;i++){char line[32];snprintf(line,sizeof line,"Metadata line %d\n",i);strcat(s.descricao,line);}
  strcat(s.descricao,s.arquivo);
  int n=linhasAddon(&s,body,FOLHA_ADDON_LINHAS);
  assert(n==FOLHA_ADDON_LINHAS);
  assert(!strcmp(pecaAddonTexto(&body[n-1],0),s.arquivo));
  snprintf(s.descricao,sizeof s.descricao,"📁 %s",s.arquivo);
  n=linhasAddon(&s,body,FOLHA_ADDON_LINHAS);
  assert(n==1 && !strcmp(pecaAddonTexto(&body[0],0),s.arquivo));
  assert(body[0].ic[0] && !strcmp(body[0].ic[0],"emoji_1f4c1"));
  puts("PASS: complete long filename retained beyond formatter line limit, no duplicate inside window");
}
