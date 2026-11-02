#!/usr/bin/env python3
"""Real badge worker/readiness and addon draw policy, without SDL or network."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/streams.c").read_text()
draw = (root / "src/stream_formatter_draw.inc").read_text()


def function(signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


fixture = r'''
#include "selospacote.h"
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
 char url[64], rotulo[32]; unsigned short selosPacote[16];
 unsigned char nSelosPacote; unsigned selosPacoteVer;
 unsigned long long badges; int dolbyVision;
} Stream;
static Stream rows[1001], *lista=rows;
static int n=2, pack=0, available=1, schedules;
static unsigned version=7, listaGeracao=1, selosPedVer, selosPedGer;
static pthread_mutex_t verTrava=PTHREAD_MUTEX_INITIALIZER;
int selospacote_ativo(void){return pack;}
unsigned selospacote_versao(void){return version;}
int selospacote_embutidos_ok(void){return available;}
void selospacote_colorido(int c){(void)c;}
static int ajustes_selos_coloridos(void){return 0;}
static void selosAgendar(int start,int count){assert(start==0&&count==n);schedules++;selosPedVer=version;selosPedGer=listaGeracao;}
@STATE@
typedef struct { unsigned ger; int ini,k; Stream v[]; } SelosLote;
static int calculations, scenario;
static void selosCalcular(Stream *s){
 if(calculations==1 && scenario==0)assert(lista[0].selosPacoteVer==version);
 s->nSelosPacote=1;s->selosPacote[0]=1;s->selosPacoteVer=version;
 calculations++;
 if(scenario==1)version++;
 if(scenario==2)listaGeracao++;
}
@WORKER@
@NO_WORK@
static SeloFiltro filters[]={
 {.nome="",.imagem=""},
 {.nome="AC3",.imagem="a.png",.arte=SELO_ARTE_COR},
 {.nome="5.1",.imagem="b.png",.arte=SELO_ARTE_COR},
 {.nome="4K",.imagem="r.png",.arte=SELO_ARTE_COR,.resolucao=1}
};
static int loaded[4], failed[4], requested[4];
const SeloFiltro *selospacote_filtro(unsigned short id){return id<4?&filters[id]:NULL;}
static int imageId(const char *p){return p[0]=='a'?1:p[0]=='b'?2:3;}
static unsigned tex_obter_larg(const char *p,float width){assert(width==128);int id=imageId(p);requested[id]++;return loaded[id];}
static int tex_falhou(const char *p){return failed[imageId(p)];}
@IMAGES@
@VIEW@
static int drawnPack, drawnLegacy;
static float desenharSelosPacote(const Stream *s,float x,float y,float w,float h,float a,float t,int skip){(void)s;(void)x;(void)y;(void)w;(void)h;(void)a;(void)t;(void)skip;drawnPack++;return 20;}
static unsigned long long logosDa(const Stream *s,int skip){(void)skip;return s->badges;}
static unsigned long long badges_bit(const char *p){(void)p;return 1;}
static float badges_desenhar_selos(unsigned long long m,float x,float y,float w,float h,float a){(void)m;(void)x;(void)y;(void)w;(void)h;(void)a;drawnLegacy++;return 20;}
static float badges_desenhar_tom(unsigned long long m,float x,float y,float w,float h,float r,float g,float b,float a){(void)r;(void)g;(void)b;return badges_desenhar_selos(m,x,y,w,h,a);}
static int badges_fonte_hdr_marca(unsigned long long m){(void)m;return 1;}
enum {FMT_SDR};
#define FOLHA_SELO_H 24
static void marca_formato(int f,float x,float y,float h,float r,float g,float b,float a){(void)f;(void)x;(void)y;(void)h;(void)r;(void)g;(void)b;(void)a;}
static void drawRow(Stream *s){float tx=0,y=0,bw=800,anim=1;int sel=1;
@DRAW@
}
static SelosLote *job(int count){SelosLote *l=calloc(1,sizeof *l+count*sizeof(Stream));assert(l);l->ger=listaGeracao;l->k=count;return l;}
int main(void){
 int ready=9;strcpy(rows[0].url,"a");strcpy(rows[0].rotulo,"one");rows[0].badges=1;
 assert(selosPacoteEstado(rows,&ready)==0&&!ready&&schedules==1);
 drawRow(rows);assert(!drawnPack&&drawnLegacy==1&&schedules==1);
 rows[0].selosPacoteVer=version;rows[0].nSelosPacote=1;rows[0].selosPacote[0]=1;
 drawRow(rows);assert(!drawnPack&&drawnLegacy==2&&requested[1]);
 loaded[1]=1;drawRow(rows);assert(!drawnPack&&drawnLegacy==3);
 selosVistaLimpar();drawRow(rows);assert(drawnPack==1&&drawnLegacy==3);
 selosVistaLimpar();rows[0].nSelosPacote=0;drawRow(rows);assert(drawnLegacy==4);
 selosVistaLimpar();pack=-1;available=0;rows[0].selosPacoteVer=0;drawRow(rows);assert(drawnLegacy==5);
 pack=0;available=1;
 rows[0].selosPacoteVer=version;rows[0].nSelosPacote=2;rows[0].selosPacote[0]=1;rows[0].selosPacote[1]=2;
 assert(!selosImagensProntas(rows,0)&&requested[2]);
 failed[2]=1;assert(selosImagensProntas(rows,0));
 rows[0].selosPacote[1]=3;assert(selosImagensProntas(rows,1)&&!requested[3]);
 Stream copy=rows[0];copy.selosPacoteVer=0;
 assert(selosPacoteEstado(&copy,&ready)==2&&ready);
 n=0;copy.selosPacoteVer=0;assert(selosPacoteEstado(&copy,&ready)==0&&ready);
 n=1000;memset(rows,0,sizeof rows);calculations=0;selosFio(job(n));assert(calculations==1000&&rows[999].selosPacoteVer==version);
 // A stale pack result must not erase a newer result already in the list.
 n=2;scenario=1;calculations=0;rows[0].selosPacoteVer=version+1;rows[0].nSelosPacote=3;selosFio(job(1));assert(rows[0].nSelosPacote==3);
 scenario=2;calculations=0;selosFio(job(1000));assert(calculations==1);
 rows[0].selosPacoteVer=0;selosSemTrabalho(0,1,listaGeracao,version);assert(rows[0].selosPacoteVer==version&&rows[0].nSelosPacote==0);
 rows[0].nSelosPacote=2;selosSemTrabalho(0,1,listaGeracao-1,version);assert(rows[0].nSelosPacote==2);
 selosVistaLimpar();scenario=0;n=1000;
 for(int i=0;i<n;i++){rows[i].selosPacoteVer=version;rows[i].nSelosPacote=1;rows[i].selosPacote[0]=1;assert(selosVistaDa(&rows[i],0)==SELOS_VISTA_PACOTE);}
 int requestsBefore=requested[1], schedulesBefore=schedules;
 struct timespec begin,end;clock_gettime(CLOCK_MONOTONIC,&begin);
 volatile int result=0;
 for(int i=0;i<2000000;i++)result=selosVistaDa(&rows[i%1000],0);
 clock_gettime(CLOCK_MONOTONIC,&end);
 assert(result==SELOS_VISTA_PACOTE&&requested[1]==requestsBefore&&schedules==schedulesBefore);
 printf("badge view cache: 1000 cards, 2M hits, %.1f ns/hit, no repeated texture requests or matching\n",((end.tv_sec-begin.tv_sec)*1e9+end.tv_nsec-begin.tv_nsec)/2000000.0);
 assert(selosVistaCap<=2000);selosVistaLimpar();assert(!selosVista&&!selosVistaCap);
 puts("PASS: immediate stable badges, next-open final pack, 1000-row incremental publication, stale jobs, failures and bounded view cache");
}
'''
fixture = fixture.replace("@STATE@", function("static int selosPacoteEstado("))
fixture = fixture.replace("@WORKER@", function("static void *selosFio("))
fixture = fixture.replace("@NO_WORK@", function("static void selosSemTrabalho("))
fixture = fixture.replace("@IMAGES@", function("static int selosImagensProntas("))
view_start=source.index("enum { SELOS_VISTA_LEGADO")
view_end=source.index("static float desenharSelosPacote(",view_start)
fixture=fixture.replace("@VIEW@",source[view_start:view_end])
start = draw.index("  if(selosVistaDa(")
end = draw.index("  y+=FOLHA_ADDON_SELOS;", start)
fixture = fixture.replace("@DRAW@", draw[start:end])
with tempfile.TemporaryDirectory(prefix="nuvio-badge-ready-") as directory:
    temp = Path(directory)
    (temp / "test.c").write_text(fixture)
    flags = ["-std=gnu11", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-pthread"]
    if os.environ.get("SANITIZE") == "1":
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    binary = str(temp / "test")
    subprocess.run([os.environ.get("CC", "cc"), *flags, "-I" + str(root / "src"),
                    str(temp / "test.c"), "-o", binary], check=True)
    subprocess.run([binary], check=True)
