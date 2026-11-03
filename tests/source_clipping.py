#!/usr/bin/env python3
"""Compare the actual inline addon drawing with/without offscreen culling."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
s = (ROOT / "src/streams.c").read_text()
start = s.index('    if(ajustes_fonte_texto_addon()) {\n      cy+=desenharFaixas')
end = s.index('      continue;\n    }', start)
body = s[start:end] + '\n    }'
original = re.sub(r'        // Keep partially visible.*?        if \(ly > folhaBase\) break;\n', '', body, flags=re.S)
assert original != body
fixture = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {float x,y,w,h;} GfxRect;
typedef struct {int w,h;} TxtLinha;
typedef struct {int n;const char *ic[2];char tx[2][80];} LinhaAddon;
typedef struct {int nl;LinhaAddon linhas[24];} TextoAddon;
static TextoAddon data;
#define FOLHA_ADDON_FECHADA 3
#define FOLHA_ADDON_LD 28.0f
#define FOLHA_TOPO 249.25f
enum {TXT_PG_FIM};
static float folhaBase=972.75f,scale=1;
static int grupo=1,foco,linhaAnt=-1,fontHeight=29,calls,lookups,visibleCount;
static float abreFoco=1,abreAnt;
static int ajustes_fonte_texto_addon(void){return 1;}
static float gfx_escala_ui(void){return scale;}
static float desenharFaixas(int s,float x,float y,float w,int sel,float a){(void)s;(void)x;(void)y;(void)w;(void)sel;(void)a;return 0;}
static const TextoAddon *textoAddonDa(int i){(void)i;return &data;}
typedef struct {GfxRect r;float red,green,blue,alpha;int icon;} Draw;
static Draw visible[512],reference[512];
static void record(GfxRect r,float red,float green,float blue,float alpha,int icon){
 calls++;if(r.y+r.h<=FOLHA_TOPO || r.y>=folhaBase)return;
 assert(visibleCount<512);visible[visibleCount++]=(Draw){r,red,green,blue,alpha,icon};
}
static void gfx_icone(GfxRect r,const char *name,float red,float green,float blue,float alpha){(void)name;record(r,red,green,blue,alpha,1);}
static TxtLinha txt_linha_corta(int style,const char *s,int r,int g,int b,int a,float max){
 (void)style;(void)r;(void)g;(void)b;(void)a;lookups++;
 return (TxtLinha){(int)fminf(max,strlen(s)*7),fontHeight};
}
static void txt_desenhar_tom(TxtLinha l,float x,float y,float r,float g,float b,float a){record((GfxRect){x,y,l.w,l.h},r,g,b,a,0);}
static void fast(float cy,int sel){int s=0,i=0,row=0;float tx=90,tr=690,txtW=600,anim=1;@FAST@}
static void legacy(float cy,int sel){int s=0,i=0,row=0;float tx=90,tr=690,txtW=600,anim=1;@OLD@}
int main(void){
 data.nl=24;
 for(int i=0;i<24;i++){data.linhas[i].n=2;data.linhas[i].ic[0]="emoji";strcpy(data.linhas[i].tx[0],"REMUX DTS-HD MA 5.1 155.48 GB");strcpy(data.linhas[i].tx[1],"filename.mkv");}
 int reduced=0;
 for(int size=0;size<3;size++)for(int sel=0;sel<2;sel++)for(int offset=-120;offset<120;offset++){
   scale=size==0?.7f:size==1?1:1.5f;fontHeight=size==0?22:size==1?29:48;
   float y=offset*11.13f;
   calls=lookups=visibleCount=0;legacy(y,sel);int count=visibleCount,oldCalls=calls,oldLookups=lookups;
   memcpy(reference,visible,sizeof reference);
   calls=lookups=visibleCount=0;fast(y,sel);
   assert(visibleCount==count);
   for(int i=0;i<count;i++)assert(!memcmp(&visible[i],&reference[i],sizeof(Draw)));
   assert(calls<=oldCalls && lookups<=oldLookups);if(calls<oldCalls)reduced++;
 }
 assert(reduced>0);puts("PASS: visible upstream formatter text/icons unchanged in 1440 scroll/font/focus cases; fewer offscreen draws and text lookups");
}
'''.replace('@FAST@', body).replace('@OLD@', original)
with tempfile.TemporaryDirectory(prefix='nuvio-source-clip-') as directory:
    path = Path(directory) / 'test.c'
    path.write_text(fixture)
    flags = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror']
    if os.environ.get('SANITIZE') == '1':
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    binary = str(Path(directory) / 'test')
    subprocess.run([os.environ.get('CC', 'cc'), *flags, str(path), '-lm', '-o', binary], check=True)
    subprocess.run([binary], check=True)
