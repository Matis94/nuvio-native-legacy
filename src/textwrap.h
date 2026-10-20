#ifndef NV_TEXTWRAP_H
#define NV_TEXTWRAP_H
#include <stddef.h>
#include <string.h>

/* One lossless UTF-8 line, including filenames with no spaces. The same
 * splitter is used for measuring and drawing. Buffers need at least 5 bytes. */
static inline const char *nv_wrap_line(const char *s, char *out, size_t cap,
                                      float width, int (*measure)(const char *, void *), void *ctx) {
  size_t ends[512], count=0, bytes=0, lo=0, hi, fit;
  if (!s || !out || cap<5) return s;
  if (*s=='\n') { out[0]=0; return s+1; }
  while (s[bytes] && s[bytes]!='\n' && count<512) {
    size_t n=1;
    /* Consume the complete code point, never cut a UTF-8 continuation. */
    while (s[bytes+n] && ((unsigned char)s[bytes+n]&0xc0)==0x80) n++;
    if (bytes+n>=cap) break;
    bytes+=n; ends[count++]=bytes;
  }
  hi=count;
  while (lo<hi) {
    size_t mid=lo+(hi-lo+1)/2;
    memcpy(out,s,ends[mid-1]); out[ends[mid-1]]=0;
    if (measure(out,ctx)<=width) lo=mid; else hi=mid-1;
  }
  /* Always advance, including when the available width is narrower than a glyph. */
  fit=lo?ends[lo-1]:count?ends[0]:1;
  if (fit<bytes || (s[fit] && s[fit]!='\n')) {
    size_t boundary=0;
    for (size_t i=0;i<fit;i++)
      if (s[i]==' ' || s[i]=='.' || s[i]=='-' || s[i]=='_' || s[i]=='/') boundary=i+1;
    if (boundary>fit/2) fit=boundary;
  }
  memcpy(out,s,fit); out[fit]=0;
  s+=fit;
  if (*s=='\n') s++;
  return s;
}
#endif
