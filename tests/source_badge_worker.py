#!/usr/bin/env python3
"""Exercise the actual bounded badge worker across append/replace/pack races."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
s = (ROOT / "src/streams.c").read_text()
start = s.index("typedef struct { unsigned ger, ver; int ini, k; Stream v[1]; } SelosLote;")
worker = s[start:s.index("static void selosAgendar(int ini, int qtd) {", start)]
worker = worker.replace("static unsigned selosPedVer, selosPedGer;", "")
scheduler = s[s.index("static void selosAgendar(int ini, int qtd) {"):s.index("static int selosPacoteDa(Stream *s) {")]
assert "malloc(sizeof *L)" in scheduler and "memcpy(L->v" not in scheduler
fixture = r'''
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { char url[8192]; unsigned selosPacote[12],nSelosPacote,selosPacoteVer; } Stream;
static Stream *lista; static int n; static unsigned listaGeracao=1,version=1;
static pthread_mutex_t verTrava=PTHREAD_MUTEX_INITIALIZER;
static int mode,computed;
static unsigned selospacote_versao(void){return version;}
static void selosCalcular(Stream *s){
  // Regex must run without holding the list mutex, with its own scratch row.
  assert(!pthread_mutex_trylock(&verTrava));
  assert(s<lista || s>=lista+n);
  computed++;
  if(computed==1){
    if(mode==1){lista=realloc(lista,2000*sizeof *lista);assert(lista);n=2000;}
    if(mode==2){free(lista);lista=calloc(8,sizeof *lista);n=8;listaGeracao++;}
    if(mode==3)version++;
  }
  pthread_mutex_unlock(&verTrava);
  s->selosPacote[0]=42;s->nSelosPacote=1;s->selosPacoteVer=version;
}
@WORKER@
int main(void){
  for(mode=0;mode<5;mode++){
    n=1950;lista=calloc(2000,sizeof *lista);assert(lista);computed=0;listaGeracao++;version++;
    SelosLote *job=calloc(1,sizeof *job);assert(job);
    assert(sizeof *job<2*sizeof(Stream));
    job->ger=listaGeracao;job->ver=version;job->ini=0;job->k=n;
    if(mode==4){version++;}
    pthread_t worker;assert(!pthread_create(&worker,NULL,selosFio,job));assert(!pthread_join(worker,NULL));
    if(mode==0 || mode==1){
      assert(computed==1950);
      for(int i=0;i<1950;i++)assert(lista[i].selosPacote[0]==42 && lista[i].selosPacoteVer==version);
      if(mode==1)for(int i=1950;i<n;i++)assert(!lista[i].selosPacoteVer);
    }else{
      assert(computed==(mode==4?0:1));
      for(int i=0;i<n;i++)assert(!lista[i].selosPacoteVer);
    }
    free(lista);lista=NULL;
  }
  puts("PASS: one-row badge worker, regex outside list lock, append/reallocation, replacement and stale pack cancellation");
}
'''.replace("@WORKER@", worker)
with tempfile.TemporaryDirectory(prefix="nuvio-source-badge-worker-") as directory:
    path = Path(directory) / "test.c"
    path.write_text(fixture)
    flags = ["-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-pthread"]
    if os.environ.get("SANITIZE") == "1":
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    binary = str(Path(directory) / "test")
    subprocess.run([os.environ.get("CC", "cc"), *flags, str(path), "-o", binary], check=True)
    subprocess.run([binary], check=True)
