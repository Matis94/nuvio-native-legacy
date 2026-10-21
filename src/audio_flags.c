#include "audio_flags.h"
#include "linguas.h"
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

static const struct { const char *nome, *iso, *iso3, *alias, *pais; } idiomas[] = {
  {"English","en","eng","","gb"}, {"Japanese","ja","jpn","","jp"},
  {"Chinese","zh","zho","chi","cn"}, {"Russian","ru","rus","","ru"},
  {"Arabic","ar","ara","","sa"}, {"Portuguese","pt","por","","pt"},
  {"Spanish","es","spa","esp","es"}, {"French","fr","fra","fre","fr"},
  {"German","de","deu","ger","de"}, {"Italian","it","ita","","it"},
  {"Korean","ko","kor","","kr"}, {"Hindi","hi","hin","","in"},
  {"Bengali","bn","ben","","bd"}, {"Punjabi","pa","pan","","pk"},
  {"Marathi","mr","mar","","in"}, {"Gujarati","gu","guj","","in"},
  {"Tamil","ta","tam","","in"}, {"Telugu","te","tel","","in"},
  {"Kannada","kn","kan","","in"}, {"Malayalam","ml","mal","","in"},
  {"Thai","th","tha","","th"}, {"Vietnamese","vi","vie","","vn"},
  {"Indonesian","id","ind","","id"}, {"Turkish","tr","tur","","tr"},
  {"Hebrew","he","heb","","il"}, {"Persian","fa","fas","per","ir"},
  {"Ukrainian","uk","ukr","","ua"}, {"Greek","el","ell","gre","gr"},
  {"Lithuanian","lt","lit","","lt"}, {"Latvian","lv","lav","","lv"},
  {"Estonian","et","est","","ee"}, {"Polish","pl","pol","","pl"},
  {"Czech","cs","ces","cze","cz"}, {"Slovak","sk","slk","slo","sk"},
  {"Hungarian","hu","hun","","hu"}, {"Romanian","ro","ron","rum","ro"},
  {"Bulgarian","bg","bul","","bg"}, {"Serbian","sr","srp","","rs"},
  {"Croatian","hr","hrv","","hr"}, {"Slovenian","sl","slv","","si"},
  {"Dutch","nl","nld","dut","nl"}, {"Danish","da","dan","","dk"},
  {"Finnish","fi","fin","","fi"}, {"Swedish","sv","swe","","se"},
  {"Norwegian","no","nor","nb","no"}, {"Malay","ms","msa","may","my"},
  {"Icelandic","is","isl","ice","is"}, {"Filipino","tl","tgl","fil","ph"}
};
static const char *paisDoIdioma(const char *raw) {
  static const char *const br[] = {"pob","ptb","pb","br","ptbr","pt-br","pt_br",
    "Brazilian Portuguese","Portuguese (Brazil)","Portuguese (Brasil)","Portuguese (BR)",
    "Português (BR)","Português (Brasil)"};
  for (unsigned i=0;i<sizeof br/sizeof *br;i++) if (!strcasecmp(raw,br[i])) return "br";
  if ((!strncasecmp(raw,"pt-br",5) && (raw[5]=='-' || raw[5]=='_')) ||
      !strcasecmp(raw,"por-BR")) return "br";
  if (!strcasecmp(raw,"Portuguese (Portugal)") || !strcasecmp(raw,"Portuguese (PT)") ||
      !strcasecmp(raw,"Português (PT)") || !strcasecmp(raw,"Português (Portugal)")) return "pt";
  char base[4]; size_t n=strcspn(raw,"-_");
  if (n>=2 && n<=3) {
    for(size_t i=0;i<n;i++) base[i]=(char)tolower((unsigned char)raw[i]);
    base[n]=0;
  } else base[0]=0;
  for(unsigned i=0;i<sizeof idiomas/sizeof *idiomas;i++) {
    if (!strcasecmp(raw,idiomas[i].nome) || !strcasecmp(raw,ling_nome(idiomas[i].iso)) ||
        (base[0] && (!strcmp(base,idiomas[i].iso) || !strcmp(base,idiomas[i].iso3) ||
                     (idiomas[i].alias[0] && !strcmp(base,idiomas[i].alias))))) return idiomas[i].pais;
  }
  return NULL;
}
void audio_flags_ler(const Stream *s, AudioFlags *out) {
  memset(out,0,sizeof *out);
  if (!s) return;
  size_t np=strnlen(s->audioBandeiras,sizeof s->audioBandeiras);
  for(size_t k=0;k+1<np && out->n<AUDIO_FLAGS_MAX;k+=2) {
    if(!islower((unsigned char)s->audioBandeiras[k]) || !islower((unsigned char)s->audioBandeiras[k+1]))continue;
    char pais[3]={s->audioBandeiras[k],s->audioBandeiras[k+1],0};
    unsigned j=0; while(j<out->n && strcmp(out->pais[j],pais))j++;
    if(j==out->n)strcpy(out->pais[out->n++],pais);
  }
  const char *p=s->audioIdiomas, *fim=p+strnlen(p,sizeof s->audioIdiomas);
  while(p<fim) {
    const char *e=memchr(p,',',(size_t)(fim-p)); if(!e)e=fim;
    while(p<e && isspace((unsigned char)*p))p++;
    const char *ate=e; while(ate>p && isspace((unsigned char)ate[-1]))ate--;
    char raw[512]; snprintf(raw,sizeof raw,"%.*s",(int)(ate-p),p);
    char bandeira[3]={0};
    if(strlen(raw)==8 && !memcmp(raw,"\xF0\x9F\x87",3) && !memcmp(raw+4,"\xF0\x9F\x87",3) &&
       (unsigned char)raw[3]>=0xA6 && (unsigned char)raw[3]<=0xBF &&
       (unsigned char)raw[7]>=0xA6 && (unsigned char)raw[7]<=0xBF) {
      bandeira[0]=(char)('a'+(unsigned char)raw[3]-0xA6);
      bandeira[1]=(char)('a'+(unsigned char)raw[7]-0xA6);
    }
    const char *pais=bandeira[0]?bandeira:paisDoIdioma(raw);
    if(pais) {
      unsigned j=0; while(j<out->n && strcmp(out->pais[j],pais))j++;
      if(j==out->n && out->n<AUDIO_FLAGS_MAX) strcpy(out->pais[out->n++],pais);
    } else if(raw[0] && strcasecmp(raw,"und") && strcasecmp(raw,"unknown")) out->outros++;
    p=e<fim?e+1:fim;
  }
}
