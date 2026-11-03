#!/usr/bin/env python3
"""Differential source ordering and clipped formatter drawing, without SDL/HTTP."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


source = (ROOT / "src/streams.c").read_text()
draw = (ROOT / "src/stream_formatter_draw.inc").read_text()
fixture = r'''
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define CAP 70032
static unsigned keys[CAP], *chave=keys;
static int order[CAP], *exib=order, expected[CAP], scratch[CAP];
typedef struct {long tamanhoMB;} Stream;
static Stream rows[CAP], *lista=rows;
@APPEND@
@SIZE@
static unsigned long sizeCalls;
static long countedSize(int i){sizeCalls++;return tamanhoOrdem(i);}
#define tamanhoOrdem countedSize
@SORT@
#undef tamanhoOrdem
static unsigned randomState=314159;
static unsigned nextRandom(void){randomState=randomState*1664525u+1013904223u;return randomState;}
static void legacyAppend(int before,int added){
 for(int k=0;k<added;k++){
  int i=before+k,pos=i;
  while(pos>0 && keys[expected[pos-1]]>keys[i]){expected[pos]=expected[pos-1];pos--;}
  expected[pos]=i;
 }
}
static void append(int before,int added,int rank){
 for(int i=0;i<added;i++)keys[before+i]=((unsigned)(rank<0?0:rank)<<16)|(unsigned)(i&0xffff);
 legacyAppend(before,added);ordemAcrescentar(before,added);
 assert(!memcmp(order,expected,(size_t)(before+added)*sizeof *order));
 for(int i=1;i<before+added;i++)assert(keys[order[i-1]]<=keys[order[i]]);
}
static void appendCases(void){
 append(0,262,4);append(262,1607,0);append(1869,25,2);
 append(1894,57,0);append(1951,0,1);append(1951,49,-1);
 for(int round=0;round<120;round++){
  int count=0;
  for(int batch=0;batch<12;batch++){
   int amount=(int)(nextRandom()%180);
   append(count,amount,(int)(nextRandom()%7));count+=amount;
  }
 }
 // Exercise the original 16-bit key limit and its wrapped fallback, too.
 append(0,65536,0);append(65536,1,0);
 append(0,65537,0);
 puts("PASS: late addon batches, duplicate keys, stable ties, empty batches and 65536-key boundary match legacy order");
}
static unsigned long legacySize(int *v,int count){
 unsigned long calls=0;
 for(int k=1;k<count;k++){
  int value=v[k],j=k-1;
  while(j>=0){calls+=2;if(tamanhoOrdem(v[j])>=tamanhoOrdem(value))break;v[j+1]=v[j];j--;}
  v[j+1]=value;
 }
 return calls;
}
static void sizes(void){
 unsigned long oldWork=0,newWork=0;
 for(int round=0;round<150;round++){
  int count=round==0?1950:(int)(nextRandom()%5001);
  for(int i=0;i<count;i++){
   order[i]=expected[i]=i;
   rows[i].tamanhoMB=round==0?i:(long)(nextRandom()%31)-3;
   if(round && i%19==0)rows[i].tamanhoMB=LONG_MAX;
  }
  unsigned long old=legacySize(expected,count);sizeCalls=0;
  ordenarTamanho(order,scratch,count);
  assert(!memcmp(order,expected,(size_t)count*sizeof *order));
  if(round==0){oldWork=old;newWork=sizeCalls;}
 }
 assert(newWork*20<oldWork);
 printf("PASS: descending sizes/unknowns/LONG_MAX preserve all ties; 1950-row adversarial size lookups %lu -> %lu\n",oldWork,newWork);
}
typedef struct {float x,y,w,h;} GfxRect;
typedef struct {unsigned tex;int w,h,pw,ph;} TxtLinha;
typedef int TxtEstilo;
enum {TXT_CALLOUT,TXT_PG_FIM};
#define FOLHA_ADDON_LD 28.0f
typedef struct {int n;const char *ic[24];unsigned short off[24];char texto[2048];} LinhaAddon;
typedef struct {unsigned short inicio,tam;unsigned char linha,peca,visual,icone;float x,w;} AddonDesenho;
static const char *pecaAddonTexto(const LinhaAddon *l,int p){return l->texto+l->off[p];}
static float top,bottom;
static int calls,lookups,nVisible,fontHeight=29;
typedef struct {float x,y,w,h,r,g,b,a;int icon;} DrawCall;
static DrawCall visible[512], reference[512];
static void record(GfxRect r,float red,float green,float blue,float alpha,int icon){
 calls++;
 if(r.y+r.h<=top || r.y>=bottom)return;
 assert(nVisible<512);
 visible[nVisible++]=(DrawCall){r.x,r.y,r.w,r.h,red,green,blue,alpha,icon};
}
static int gfx_icone_cor(GfxRect r,const char *s,float a){(void)s;record(r,1,1,1,a,1);return 1;}
static const char *emojiReserva(const char *s){return s;}
static void gfx_icone(GfxRect r,const char *s,float red,float green,float blue,float a){(void)s;record(r,red,green,blue,a,1);}
static TxtLinha txt_linha(TxtEstilo e,const char *s,int r,int g,int b,int a){
 (void)e;(void)r;(void)g;(void)b;(void)a;lookups++;return (TxtLinha){1,(int)strlen(s)*7,fontHeight,0,0};
}
static TxtLinha txt_linha_corta(TxtEstilo e,const char *s,int r,int g,int b,int a,float max){
 TxtLinha t=txt_linha(e,s,r,g,b,a);if(t.w>max)t.w=(int)max;return t;
}
static void txt_desenhar_tom(TxtLinha t,float x,float y,float r,float g,float b,float a){record((GfxRect){x,y,t.w,t.h},r,g,b,a,0);}
static float gfx_px_por_unidade(void){return 1;}
static float gfx_escala(void){return 1;}
@PIXEL@
@DRAW@
static void clippedDraw(void){
 LinhaAddon lines[12]={0};AddonDesenho pieces[256];int count=0;
 for(int i=0;i<12;i++){
  strcpy(lines[i].texto,"Cloud Polish English DUAL MULTI REMUX DTS-HD MA 5.1 155.48 GB filename.mkv");
  lines[i].ic[0]="emoji";
  for(int row=0;row<8;row++){
   pieces[count++]=(AddonDesenho){0,0,(unsigned char)i,0,(unsigned char)(i*8+row),1,0,22};
   pieces[count++]=(AddonDesenho){0,70,(unsigned char)i,0,(unsigned char)(i*8+row),0,30,490};
  }
 }
 int reduced=0;
 for(int font=0;font<3;font++)for(int selected=0;selected<2;selected++)for(int offset=-120;offset<120;offset++){
  fontHeight=font==0?22:font==1?29:48;top=249.25f;bottom=972.75f;
  float y=offset*11.13f;
  calls=lookups=nVisible=0;
  addonDesenharPecas(lines,pieces,count,90,y,600,96*28,TXT_PG_FIM,selected,1,-INFINITY,INFINITY);
  int oldCalls=calls,oldLookups=lookups,oldVisible=nVisible;memcpy(reference,visible,sizeof reference);
  calls=lookups=nVisible=0;
  addonDesenharPecas(lines,pieces,count,90,y,600,96*28,TXT_PG_FIM,selected,1,top,bottom);
  assert(nVisible==oldVisible);
  for(int i=0;i<nVisible;i++){
   DrawCall *a=&visible[i],*b=&reference[i];
   assert(a->x==b->x && a->y==b->y && a->w==b->w && a->h==b->h && a->r==b->r && a->g==b->g && a->b==b->b && a->a==b->a && a->icon==b->icon);
  }
  assert(calls<=oldCalls && lookups<=oldLookups);
  if(y<0){assert(calls<oldCalls);reduced++;}
 }
 assert(reduced>0);
 puts("PASS: all visible text/emoji fragments identical at 1440 fractional scroll/font/focus cases; fewer hidden draws/lookups");
}
int main(void){appendCases();sizes();clippedDraw();return 0;}
'''
for key, signature in [("APPEND", "static void ordemAcrescentar("),
                       ("SIZE", "static long tamanhoOrdem("),
                       ("SORT", "static void ordenarTamanho(")]:
    fixture = fixture.replace("@" + key + "@", function(source, signature))
fixture = fixture.replace("@PIXEL@", function(draw, "static float addonPixel("))
fixture = fixture.replace("@DRAW@", function(draw, "static void addonDesenharPecas("))
with tempfile.TemporaryDirectory(prefix="nuvio-source-scroll-") as directory:
    temp = Path(directory)
    (temp / "test.c").write_text(fixture)
    flags = ["-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror"]
    if os.environ.get("SANITIZE") == "1":
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    binary = str(temp / "test")
    subprocess.run([os.environ.get("CC", "cc"), *flags, str(temp / "test.c"),
                    "-lm", "-o", binary], check=True)
    subprocess.run([binary], check=True)
