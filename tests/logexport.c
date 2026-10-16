#define _POSIX_C_SOURCE 200809L
#include "logexport.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void hides(const char *line, const char *secret) {
  char out[2048]; logexport_redact(line, out, sizeof out);
  assert(!strstr(out, secret));
}
int main(void) {
  char out[4096], source[] = "/tmp/nuvio-logtest-XXXXXX", path[128];
  int fd = mkstemp(source); assert(fd >= 0);
  FILE *f = fdopen(fd, "wb"); assert(f);
  const char *safe = "[playback-diag] tick=123 generation=2 pos=123.456 readyBytes=268435456 availableKiB=245760";
  logexport_redact(safe, out, sizeof out); assert(!strcmp(safe, out));
  const char *range_logs[]={
    "[range-http] start=10737418240 end=10738466815 curl=28 http=206 got=524288 valid=1 overflow=0 responseStart=10737418240 responseEnd=10738466815 total=30000000000",
    "[range-result] start=4194304 end=5242879 attempts=2 ok=1 http=206 bytes=1048576 ms=15020",
    "[range-read] pos=4194304 waitMs=2150 http=206 ok=1 readyBytes=64000000 contiguousBytes=64000000",
    "[range-reset] reason=seek pos=100000000 ready=32 readyBytes=33554432 active=4"};
  for(size_t i=0;i<sizeof range_logs/sizeof range_logs[0];i++) {
    logexport_redact(range_logs[i],out,sizeof out);assert(!strcmp(out,range_logs[i]));
  }
  hides("[net] https://person:shortpw@host.example/key/movie?api=x#y", "person");
  hides("[net] HtTpS://private.example/shortpw", "shortpw");
  hides("[net] https:\\/\\/private.example/key", "private.example");
  hides("[net] url:https://a/shortpw", "shortpw");
  hides("[addon] authorization: Basic YTpi", "YTpi");
  hides("[addon] Cookie: a=shortpw; b=123", "shortpw");
  hides("[addon] apiKey=shortpw", "shortpw");
  hides("[addon] accessToken=shortpw", "shortpw");
  hides("[addon] {\"password\":\"shortpw\"}", "shortpw");
  hides("[addon] url=http%3A%2F%2Fprivate.example%2Fshortpw", "shortpw");
  hides("[auth] pin=1234", "1234");
  hides("[auth] username=someone", "someone");
  hides("[net] contact foo@example.test", "foo@example.test");
  hides("[net] 192.168.1.100", "192.168.1.100");
  hides("[net] aa:bb:cc:dd:ee:ff", "aa:bb:cc:dd:ee:ff");
  hides("[net] 2001:db8:1234::abcd", "2001:db8:1234::abcd");
  hides("[net] 1234567890abcdef1234567890abcdef", "1234567890abcdef1234567890abcdef");
  hides("[net] /media/private-user/file.mkv", "private-user");
  hides("[net] eyJabc.def.ghi", "eyJabc");
  char tiny[3] = {1,2,3}; logexport_redact("password=shortpw", tiny, sizeof tiny); assert(!tiny[2]);
  fprintf(f, "%s\n[auth] password:  \nshortpw\n  \"secondpw\"\n[video] error=300\n", safe);
  /* An oversized line must be omitted as a whole, never export its chunks. */
  for (int i = 0; i < 70000; i++) fputc('x', f);
  fputs("ENDSECRET\n[video] recovery=1\n", f); fclose(f);
  assert(logexport_create(source, path, sizeof path));
  struct stat st; assert(!stat(path, &st) && (st.st_mode & 0777) == 0600);
  f = fopen(path, "rb"); assert(f); size_t n = fread(out, 1, sizeof out - 1, f); out[n] = 0; fclose(f);
  assert(strstr(out, safe) && strstr(out, "[video] error=300") && strstr(out, "recovery=1"));
  assert(!strstr(out, "shortpw") && !strstr(out, "secondpw") && !strstr(out, "ENDSECRET"));
  assert(strstr(out, "oversized log line")); unlink(path);
  /* Taking a tail cannot leak the partial first line. */
  f = fopen(source, "wb"); assert(f);
  for (int i = 0; i < 17 * 1024 * 1024; i++) fputc('x', f);
  fputs("TRUNCATEDSECRET\n[video] tail=1\n", f); fclose(f);
  assert(logexport_create(source, path, sizeof path));
  f = fopen(path, "rb"); assert(f); n = fread(out, 1, sizeof out - 1, f); out[n] = 0; fclose(f);
  assert(strstr(out, "tail=1") && !strstr(out, "TRUNCATEDSECRET"));
  unlink(path); unlink(source);
  assert(!logexport_create("/nonexistent/nuvio.log", path, sizeof path) && !path[0]);
  puts("PASS: log export redaction, numeric diagnostics, multiline secrets, bounds, private permissions");
}
