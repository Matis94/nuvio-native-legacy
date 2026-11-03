#!/usr/bin/env python3
"""Verify source-card text and art share a stable pixel phase while scrolling."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
draw = (ROOT/'src/stream_formatter_draw.inc').read_text()
text = (ROOT/'src/text.c').read_text()

def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

fixture = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {float x,y,w,h;} GfxRect;
typedef struct {int w,h;} TxtLinha;
typedef int TxtEstilo;
enum {TXT_CALLOUT};
#define FOLHA_ADDON_LD 28.0f
static float density=1,scale=1;
static float gfx_px_por_unidade(void){return density;}
static float gfx_escala(void){return scale;}
#define ESC_T (density*scale)
@SNAP@
@PIXEL@
@ALIGN@
typedef struct {const char *ic[2];const char *texto[2];} LinhaAddon;
typedef struct {int visual,linha,peca,icone,inicio,tam;float x,w;} AddonDesenho;
static const char *pecaAddonTexto(const LinhaAddon *line,int p){return line->texto[p];}
static const char *emojiReserva(const char *p){return p;}
static int icons,texts;
static GfxRect iconRect,textRect;
static int gfx_icone_cor(GfxRect r,const char *p,float alpha){(void)p;assert(alpha==1);iconRect=r;icons++;return 1;}
static void gfx_icone(GfxRect r,const char *p,float red,float g,float b,float a){(void)r;(void)p;(void)red;(void)g;(void)b;(void)a;assert(0);}
static TxtLinha txt_linha(TxtEstilo style,const char *s,int r,int g,int b,int a){(void)style;(void)s;(void)r;(void)g;(void)b;(void)a;return (TxtLinha){80,20};}
static TxtLinha txt_linha_corta(TxtEstilo style,const char *s,int r,int g,int b,int a,float width){(void)width;return txt_linha(style,s,r,g,b,a);}
static void txt_desenhar_tom(TxtLinha line,float x,float y,float r,float g,float b,float a){
 (void)r;(void)g;(void)b;assert(a==1);texts++;
 textRect=(GfxRect){encaixa(x),encaixa(y),(float)line.w,(float)line.h};
}
@PIECES@
static void sample(float x,float y,int aligned,int visual,int heading,float *relativeX,float *relativeY){
 GfxRect card={x,y,600,400};float left=x+26,right=x+574;
 GfxRect original=card;float width=right-left;
 if(aligned){
  addonAlinharCartao(&card,&left,&right);
  assert(card.w==original.w&&card.h==original.h);
  assert(fabsf((right-left)-width)<.001f);
  assert(fabsf((left-card.x)-26)*ESC_T<=.501f);
  assert(fabsf(card.x-original.x)*ESC_T<=.501f&&fabsf(card.y-original.y)*ESC_T<=.501f);
 }
 LinhaAddon line={.ic={"emoji",NULL},.texto={"","WEB-DL"}};
 AddonDesenho pieces[2]={
  {.visual=visual,.icone=1,.x=0,.w=22},
  {.visual=visual,.peca=1,.tam=6,.x=28,.w=80}
 };
 icons=texts=0;
 addonDesenharPecas(&line,pieces,2,left,card.y+(aligned?addonPixel(96,ESC_T):96),548,2800,heading?TXT_CALLOUT:1,1,1,-INFINITY,INFINITY);
 assert(icons==1&&texts==1);
 if(!aligned){
  textRect.y=encaixa(y+96);iconRect.y=y+99;
 }
 *relativeX=(textRect.x-iconRect.x)*ESC_T;
 *relativeY=(textRect.y-iconRect.y)*ESC_T;
}
int main(void){
 float lo=INFINITY,hi=-INFINITY;
 for(int i=0;i<100;i++){float x,y;sample(40,80+i*.01f,0,0,0,&x,&y);lo=fminf(lo,y);hi=fmaxf(hi,y);}
 assert(hi-lo>.9f); // Reproduces independent text snapping vs fractional icons.
 const float scales[]={.8f,.85f,.9f,1,1.05f,1.1f,1.15f,1.25f,1.5f,2};
 for(int d=1;d<=2;d++)for(size_t k=0;k<sizeof scales/sizeof *scales;k++)
 for(int visual=0;visual<5;visual++)for(int heading=0;heading<2;heading++){
  density=(float)d;scale=scales[k];float firstX=0,firstY=0;
  for(int frame=0;frame<600;frame++){
   float x,y;sample(40+frame*.037f,-30+frame*.139f,1,visual,heading,&x,&y);
   if(!frame){firstX=x;firstY=y;}
   if(fabsf(x-firstX)>.002f||fabsf(y-firstY)>.002f){
    fprintf(stderr,"unstable d=%d s=%.2f v=%d f=%d x=%.5f/%.5f y=%.5f/%.5f\n",d,scale,visual,frame,x,firstX,y,firstY);assert(0);
   }
  }
 }
 puts("PASS: reproduced subpixel drift; stable text/emoji phase at 10 UI scales, 1x/2x density, headings/body and fractional scrolling; drawing-only geometry");
}
'''.replace('@SNAP@', function(text, 'static float encaixa('))
fixture = fixture.replace('@PIXEL@', function(draw, 'static float addonPixel('))
fixture = fixture.replace('@ALIGN@', function(draw, 'static void addonAlinharCartao('))
fixture = fixture.replace('@PIECES@', function(draw, 'static void addonDesenharPecas('))
with tempfile.TemporaryDirectory(prefix='nuvio-formatter-motion-') as directory:
    path=Path(directory)
    (path/'test.c').write_text(fixture)
    flags=['-std=gnu11','-O1','-g','-Wall','-Wextra','-Werror']
    if os.environ.get('SANITIZE')=='1':
        flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run([os.environ.get('CC','cc'),*flags,str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
