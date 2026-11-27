// Crash dump cleanup and the core file limit (coreguarda.c).
#define _DEFAULT_SOURCE
#include "coreguarda.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

static char pasta[256];
const char *dados_dir(void) { return pasta; }

static void gravar(const char *nome, const void *b, size_t n) {
  char c[512];
  FILE *f;
  snprintf(c, sizeof c, "%s/%s", pasta, nome);
  assert((f = fopen(c, "wb")));
  assert(fwrite(b, 1, n, f) == n);
  fclose(f);
}

static int existe(const char *nome) {
  char c[512];
  struct stat st;
  snprintf(c, sizeof c, "%s/%s", pasta, nome);
  return !lstat(c, &st);
}

int main(int argc, char **argv) {
  unsigned char core[64] = { 0x7f, 'E', 'L', 'F', 1, 1, 1 }, exe[64];
  unsigned long long bytes = 0;
  struct rlimit rl;
  assert(argc == 2);
  snprintf(pasta, sizeof pasta, "%s", argv[1]);
  core[16] = 4;                       // ET_CORE
  memcpy(exe, core, sizeof exe); exe[16] = 2;   // ET_EXEC
  gravar("core", core, sizeof core);
  gravar("core.7707", core, sizeof core);
  gravar("core.12x", core, sizeof core);      // not core.<digits>
  gravar("core.", core, sizeof core);
  gravar("core-boot-done", core, sizeof core);
  gravar("core.123", exe, sizeof exe);        // ELF, but not a core
  gravar("core.456", "hello", 5);             // not ELF
  gravar("nuvio-proto", core, sizeof core);
  { char alvo[512], link[512];
    snprintf(alvo, sizeof alvo, "%s/nuvio-proto", pasta);
    snprintf(link, sizeof link, "%s/core.999", pasta);
    assert(!symlink(alvo, link)); }           // symlink: never followed

  assert(coreguarda_eh_dump(pasta, "core.7707"));
  assert(!coreguarda_eh_dump(pasta, "core.999"));
  assert(coreguarda_limpar_pasta(pasta, &bytes) == 2);
  assert(bytes == 2 * sizeof core);
  assert(!existe("core") && !existe("core.7707"));
  assert(existe("core.12x") && existe("core.") && existe("core-boot-done"));
  assert(existe("core.123") && existe("core.456") && existe("core.999") && existe("nuvio-proto"));

  gravar("core.31337", core, sizeof core);
  assert(!chdir(pasta));
  coreguarda_bloquear();
  assert(!getrlimit(RLIMIT_CORE, &rl) && rl.rlim_cur == 0);
  coreguarda_limpar();
  assert(!existe("core.31337"));
  printf("[coreguard] test: data written before _exit\n");
  coreguarda_sair();   // must flush stdout, then end with status 0
}
