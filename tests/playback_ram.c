#include "ramoverlay.h"
#include "memlog.h"
#include "gfx.h"
#include "text.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned reads, draws, panels;
static int unavailable;
static char rendered[256];
void memlog_ler(MemlogInfo *info) {
  reads++;
  *info = unavailable ? (MemlogInfo){-1,-1,-1,-1}
    : (MemlogInfo){128*1024,160*1024,600*1024,2300*1024};
}
TxtLinha txt_linha(TxtEstilo style, const char *s, int r, int g, int b, int a) {
  (void)style; (void)r; (void)g; (void)b; (void)a;
  snprintf(rendered, sizeof rendered, "%s", s);
  return (TxtLinha){0, (int)strlen(s)*10, 24, 0, 0};
}
void txt_desenhar_alpha(TxtLinha line, float x, float y, float alpha) {
  assert(line.w > 0 && x >= 0 && y >= 0 && alpha > 0); draws++;
}
void gfx_cor(GfxRect rect, float radius, float r, float g, float b, float a) {
  (void)radius; (void)r; (void)g; (void)b;
  assert(rect.w > 0 && rect.h > 0 && a > 0); panels++;
}
int main(void) {
  ramoverlay_desenhar(100,0); assert(!reads && !draws && !panels);
  ramoverlay_desenhar(100,1); assert(reads==1 && draws==2 && panels==1);
  assert(strstr(rendered,"600 / 2300 MiB"));
  ramoverlay_desenhar(1099,1); assert(reads==1);
  ramoverlay_desenhar(1100,1); assert(reads==2);
  unsigned before=draws;
  ramoverlay_desenhar(1200,0); ramoverlay_desenhar(5000,0);
  assert(reads==2 && draws==before);
  unavailable=1;
  ramoverlay_desenhar(UINT32_MAX-100,1); assert(reads==3);
  assert(strstr(rendered,"unavailable"));
  ramoverlay_desenhar(899,1); assert(reads==4); /* wrap-safe 1000 ms */
  puts("Playback RAM: off/on/off, no disabled reads, throttling, timer wrap and unknown values passed");
}
