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
  const char *file="The.Lord.of.the.Rings.2001.Extended.2160p.TrueHD.Atmos.mkv";
  for(int space=0;space<2;space++) {
    snprintf(s.arquivo,sizeof s.arquivo,"[Cloud]%s%s",space?" ":"",file);
    snprintf(s.descricao,sizeof s.descricao,"🎬 %s\n🎧 TrueHD Atmos 7.1\n💾 155.48 GB 🏷️ Cloud",file);
    strcpy(s.url,"https://fixture.invalid/cloud-source.mkv");
    strcpy(s.bingeGroup,"cloud-release");
    Stream original=s;
    n=linhasAddon(&s,body,FOLHA_ADDON_LINHAS);
    assert(n==3 && linhasAddonContar(&s)==3);
    assert(!strcmp(pecaAddonTexto(&body[0],0),file));
    assert(!memcmp(&s,&original,sizeof s));
    // A missing or different filename still gets the complete supplied name.
    strcpy(s.descricao,"🎧 TrueHD Atmos 7.1\nCloud");
    n=linhasAddon(&s,body,FOLHA_ADDON_LINHAS);
    assert(n==3 && linhasAddonContar(&s)==3);
    assert(!strcmp(pecaAddonTexto(&body[n-1],0),s.arquivo));
  }
  strcpy(s.arquivo,"Movie.[Cloud].mkv");strcpy(s.descricao,"Movie.mkv");
  assert(linhasAddon(&s,body,FOLHA_ADDON_LINHAS)==2);
  assert(linhasAddonContar(&s)==2); // Interior filename text is significant.
  puts("PASS: Cloud display-prefix comparison avoids duplicates, retains missing filenames and leaves source metadata unchanged");
  puts("PASS: complete long filename retained beyond formatter line limit, no duplicate inside window");
}
