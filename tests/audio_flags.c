#include "audio_flags.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void check(const char *fields, unsigned bits, const char *audio, const char *paises) {
  char json[8192]; Stream *v=NULL; AudioFlags f;
  snprintf(json,sizeof json,"{\"streams\":[{\"url\":\"https://fixture.invalid/movie\",%s}]}",fields);
  assert(stream_extrair(json,"AIOStreams",&v)==1);
  if(v->audioDescritores!=bits || strcmp(v->audioIdiomas,audio)) {
    fprintf(stderr,"fields=%s bits=%u expected=%u audio=%s expected=%s\n",fields,v->audioDescritores,bits,v->audioIdiomas,audio);
    abort();
  }
  audio_flags_ler(v,&f);
  assert(f.n==strlen(paises)/2);
  for(unsigned i=0;i<f.n;i++) if (memcmp(f.pais[i],paises+2*i,2)) {
    fprintf(stderr,"flag mismatch fields=%s slot=%u actual=%s expected=%.2s\n",fields,i,f.pais[i],paises+2*i);
    abort();
  }
  free(v);
}
int main(void) {
  check("\"description\":\"🎬 Movie.2026.mkv\\n🎧 DTS 5.1\\n🗣️🌍 🇬🇧  🇵🇱 DUAL Dubbed\\n💾 11 GB 🏷️ Group\"",STREAM_AUDIO_DUAL,"English,Polish","gbpl");
  check("\"description\":\"🗣️🌍 🇬🇧 🇫🇷 🇩🇪 🇯🇵 MULTI Dubbed\"",STREAM_AUDIO_MULTI,"English,French,German,Japanese","gbfrdejp");
  check("\"description\":\"🗣️🌍 🇵🇱  🇬🇧\"",0,"Polish,English","plgb");
  check("\"description\":\"🗣️🌏 🇮🇳  🇺🇸 DUAL\"",STREAM_AUDIO_DUAL,"","inus");
  check("\"description\":\"🗣️🌍 🇬🇧 Dubbed\"",0,"English","gb");
  check("\"name\":\"🇵🇱 ☁️ [TB⚡️] AIOStreams\\n4K\",\"description\":\"🗣️🌍 🇬🇧 DUAL\"",STREAM_AUDIO_DUAL,"English","gbpl");
  check("\"name\":\"🇵🇱 [TB⚡️] AIOStreams\\n1080p\"",0,"","pl");
  check("\"audioLanguages\":[\"fr\"],\"name\":\"🇵🇱 [TB⚡️] AIOStreams\"",0,"fr","plfr");
  check("\"name\":\"Provider 🇫🇷\",\"description\":\"📝 🇵🇱 DUAL Dubbed\"",0,"","");
  char escaped[5000]="\"description\":\"";
  for(int i=0;i<400;i++)strcat(escaped,"\\u65e5");
  strcat(escaped,"\\n🗣️🌍 🇵🇱 DUAL Dubbed\"");
  check(escaped,STREAM_AUDIO_DUAL,"Polish","pl");
  check("\"description\":\"🌐 🇵🇱 🇬🇧 | Dual Audio\"",STREAM_AUDIO_DUAL,"Polish,English","plgb");
  check("\"description\":\"🇵🇱 🇬🇧\"",0,"","plgb");
  check("\"description\":\"🌎 / 🇬🇧\"",STREAM_AUDIO_MULTI,"","gb");
  check("\"description\":\"🌎 English | Polish\"",0,"English,Polish","gbpl");
  check("\"description\":\"⚑ Polish | English | Multi\"",STREAM_AUDIO_MULTI,"Polish,English","plgb");
  check("\"description\":\"🔊 DTS-HD MA 5.1 ● [Dual Audio]\"",STREAM_AUDIO_DUAL,"","");
  check("\"title\":\"Dual Audio\"",STREAM_AUDIO_DUAL,"","");
  check("\"description\":\"📝 🇵🇱 | Multi\"",0,"","");
  check("\"description\":\"Subtitles: 🌎 / 🇵🇱\"",0,"","");
  check("\"description\":\"🇬🇧 Multi Subs / 🇫🇷\"",0,"","");
  check("\"description\":\"🌎 Multi Subs / 🇫🇷\"",0,"","");
  check("\"description\":\"Movie.MULTI.DUAL.2026.mkv\"",0,"","");
  check("\"description\":\"https://fixture.invalid/Multi/🇵🇱\"",0,"","");
  check("\"description\":\"🌐 🇮🇳 🇬🇧\"",0,"English","ingb");
  check("\"audioLanguages\":[\"fr\"],\"subtitleLanguages\":[\"pl\"],\"description\":\"🇵🇱 🇬🇧 | Dual Audio\"",STREAM_AUDIO_DUAL,"fr","fr");
  check("\"streamData\":{\"parsedFile\":{\"languages\":[\"Multi\",\"Dual Audio\",\"Polish\",\"en\",\"eng\"],\"subtitles\":[\"Portuguese\"]}}",
        STREAM_AUDIO_DUAL|STREAM_AUDIO_MULTI,"Polish,en,eng","plgb");
  check("\"audioLanguages\":[\"Malayalam\",\"Malay\",\"pt-BR\",\"pob\",\"pt-PT\"]",0,"Malayalam,Malay,pt-BR,pob,pt-PT","inmybrpt");
  check("\"audioLanguages\":[\"🇺🇸\",\"🇮🇳\"]",0,"🇺🇸,🇮🇳","usin");
  check("\"audioLanguages\":[\"unknown\",\"original\",\"und\",\"Multi\"],\"subtitles\":[{\"lang\":\"pl\"}]",STREAM_AUDIO_MULTI,"","");
  // Bad/truncated values cannot leak a suffix that resembles audio metadata.
  check("\"audioLanguages\":[\"Multi\\u0000\"],\"description\":\"📝 Dual Audio\"",0,"","");
  const char *sample="{\"streams\":[{\"url\":\"https://fixture.invalid/v\",\"audioLanguages\":[\"Dual Audio\",\"Multi\"],\"description\":\"🌐 🇵🇱 🇬🇧 | Multi\\n📝 🇫🇷\"}]}";
  size_t n=strlen(sample);char *prefix=malloc(n+1);assert(prefix);
  for(size_t k=0;k<=n;k++) {
    memcpy(prefix,sample,k);prefix[k]=0;
    Stream *v=NULL;int count=stream_extrair(prefix,"fixture",&v);assert(count>=0);
    for(int i=0;i<count;i++) {
      AudioFlags f;audio_flags_ler(&v[i],&f);
      assert(f.n<=AUDIO_FLAGS_MAX && memchr(v[i].audioBandeiras,0,sizeof v[i].audioBandeiras));
    }
    free(v);
  }
  free(prefix);
  puts("audio flags: formatter/structured Dual/Multi, adjacent flags, aliases, ambiguous countries, subtitle separation and provenance OK");
}
