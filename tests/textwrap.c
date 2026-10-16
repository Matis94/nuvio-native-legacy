#include "../src/textwrap.h"
#include <assert.h>
#include <stdio.h>

static int measure(const char *s,void *ctx) {
  (void)ctx;
  int w=0;
  for(size_t i=0;s[i];i++) {
    if (((unsigned char)s[i]&0xc0)==0x80) continue;
    w+=((unsigned char)s[i]<128)?5:10;
  }
  return w;
}
static void check(const char *input,float width) {
  char line[512],joined[8192]="";
  const char *p=input;
  unsigned count=0;
  while (*p) {
    const char *next=nv_wrap_line(p,line,sizeof line,width,measure,NULL);
    assert(next>p && ++count<8192);
    assert(!((unsigned char)line[0]>=128 && ((unsigned char)line[0]&0xc0)==0x80));
    assert(measure(line,NULL)<=width || strlen(line)<=4);
    strcat(joined,line); p=next;
  }
  // This fixture has no explicit newlines: no bytes may disappear on wrapping.
  assert(!strcmp(input,joined));
}
int main(void) {
  check("Film.1080p.WEB-DL.DDP5.1.H.264-Group.mkv",80);
  check("Bardzo-długa-nazwa-pliku-Łódź-1080p-WEB-DL-Żółć.mkv",65);
  check("日本語.1080p.WEB-DL.日本語.mkv",50);
  char s[2048]; memset(s,'x',sizeof s-1); s[sizeof s-1]=0;
  check(s,130); check(s,6000); check("ą",1);
  char line[512]; const char *p="one\ntwo";
  p=nv_wrap_line(p,line,sizeof line,100,measure,NULL); assert(!strcmp(line,"one"));
  p=nv_wrap_line(p,line,sizeof line,100,measure,NULL); assert(!strcmp(line,"two") && !*p);
  puts("Full filename wrapping: UTF-8, no-space names, 2047 bytes and narrow/wide layouts passed.");
}
