#!/usr/bin/env python3
"""Top/bottom badge geometry uses production layout and unchanged row budgets."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
draw = (root / "src/stream_formatter_draw.inc").read_text()
stream = (root / "src/streams.c").read_text()
formatter = (root / "src/stream_formatter.inc").read_text()
start = draw.index("static void addonPosicoes(")
opening = draw.index("{", start)
depth, end = 1, opening + 1
while depth:
    depth += (draw[end] == "{") - (draw[end] == "}")
    end += 1
defines = "\n".join(re.findall(
    r"^#define FOLHA_(?:ADDON_(?:LD|SELOS|TITULO|BASE)|FAIXA_H|SELO_H)\s+.*$",
    stream + "\n" + formatter, re.M))
fixture = r'''
#include <assert.h>
#include <stdio.h>
typedef struct {int nv,nl;} TextoAddon;
typedef struct {int tracks;} Stream;
static int linhasFaixas(const Stream *s){return s->tracks;}
@DEFINES@
@LAYOUT@
int main(void){
 for(int nv=0;nv<=96;nv++)for(int nl=0;nl<=1;nl++)for(int tracks=0;tracks<=2;tracks++)for(int mark=0;mark<=1;mark++){
  TextoAddon t={nv,nl};Stream s={tracks};
  float base=20+mark*30,title[2],body[2],badges[2];
  float content=nv*FOLHA_ADDON_LD+(!nl?tracks*FOLHA_FAIXA_H:0);
  float budget=base+FOLHA_ADDON_TITULO+FOLHA_ADDON_SELOS+content+FOLHA_ADDON_BASE;
  for(int bottom=0;bottom<2;bottom++){
   addonPosicoes(&t,&s,base,bottom,&title[bottom],&body[bottom],&badges[bottom]);
   assert(body[bottom]==title[bottom]+FOLHA_ADDON_TITULO);
   assert(body[bottom]+content<=budget);
   assert(badges[bottom]+FOLHA_SELO_H<=budget-FOLHA_ADDON_BASE);
  }
  assert(badges[0]==base && title[0]==base+FOLHA_ADDON_SELOS);
  assert(title[1]==base && badges[1]>=body[1]+content);
  assert(body[0]-body[1]==FOLHA_ADDON_SELOS);
 }
 puts("PASS: Top/Bottom with 0..96 wrapped lines, fallback languages and marked cards fits the identical height budget");
}
'''.replace("@DEFINES@", defines).replace("@LAYOUT@", draw[start:end])
with tempfile.TemporaryDirectory(prefix="nuvio-badge-position-") as directory:
    temp = Path(directory)
    (temp / "test.c").write_text(fixture)
    flags = ["-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror"]
    if os.environ.get("SANITIZE") == "1":
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    binary = str(temp / "test")
    subprocess.run([os.environ.get("CC", "cc"), *flags, str(temp / "test.c"),
                    "-o", binary], check=True)
    subprocess.run([binary], check=True)
