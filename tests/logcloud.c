/* Send the log to the cloud: the multipart body and the host's answer. */
#include "logcloud.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
  const unsigned char gz[] = { 0x1f, 0x8b, 0x08, 0x00, 0x00, 0x41, 0x00 };
  unsigned long n = 0;
  char *c = logcloud_corpo(gz, sizeof gz, "BOUND", &n);
  assert(c && n > sizeof gz);
  assert(!memcmp(c, "--BOUND\r\nContent-Disposition: form-data; name=\"reqtype\"\r\n\r\nfileupload\r\n", 70));
  assert(strstr(c, "name=\"time\"\r\n\r\n72h\r\n"));
  const char *f = strstr(c, "filename=\"nuvio-diagnostics.txt.gz\"\r\nContent-Type: application/gzip\r\n\r\n");
  assert(f);
  f = strstr(f, "\r\n\r\n") + 4;
  assert(!memcmp(f, gz, sizeof gz));                       /* binary body kept, NULs included */
  assert(!memcmp(c + n - 13, "\r\n--BOUND--\r\n", 13));
  free(c);
  char u[128];
  assert(logcloud_resposta_url("https://litter.catbox.moe/abc123.gz", u, sizeof u) && !strcmp(u, "https://litter.catbox.moe/abc123.gz"));
  assert(logcloud_resposta_url("\n  https://litter.catbox.moe/x.gz\r\n", u, sizeof u) && !strcmp(u, "https://litter.catbox.moe/x.gz"));
  assert(!logcloud_resposta_url("File too big", u, sizeof u) && !u[0]);
  assert(!logcloud_resposta_url("http://plain.example/x", u, sizeof u));
  assert(!logcloud_resposta_url("https://", u, sizeof u));
  assert(!logcloud_resposta_url("https://litter.catbox.moe/very-long-name.gz", u, 16));
  assert(!logcloud_resposta_url(NULL, u, sizeof u));
  puts("PASS: log to the cloud (multipart body, host answer)");
  return 0;
}
