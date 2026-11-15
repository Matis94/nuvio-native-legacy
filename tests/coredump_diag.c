// The ELF notes of a core file: process name and signal (coredump_diag.c).
#include "coredump_diag.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *dados_dir(void) { return "/tmp/cdt"; }

static void w16(unsigned char *p, unsigned v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
static void w32(unsigned char *p, unsigned v) { w16(p, v & 0xffff); w16(p + 2, v >> 16); }

/* A minimal 32-bit ARM core: ELF header, one PT_NOTE, NT_PRSTATUS + NT_PRPSINFO. */
static size_t core32(unsigned char *b, int sig, const char *nome) {
  size_t o = 52 + 32, n0 = o;
  memset(b, 0, 1024);
  memcpy(b, "\x7f" "ELF", 4); b[4] = 1; b[5] = 1; b[6] = 1;
  w16(b + 16, 4); w16(b + 18, 40); w32(b + 28, 52); w16(b + 42, 32); w16(b + 44, 1);
  /* NT_PRSTATUS: namesz 5 ("CORE"), descsz 148 */
  w32(b + o, 5); w32(b + o + 4, 148); w32(b + o + 8, 1); memcpy(b + o + 12, "CORE", 5);
  w32(b + o + 20, (unsigned)sig); w16(b + o + 20 + 12, (unsigned)sig); o += 20 + 148;
  /* NT_PRPSINFO: descsz 124, pr_fname at 28 */
  w32(b + o, 5); w32(b + o + 4, 124); w32(b + o + 8, 3); memcpy(b + o + 12, "CORE", 5);
  memcpy(b + o + 20 + 28, nome, strlen(nome)); o += 20 + 124;
  w32(b + 52, 4); w32(b + 52 + 4, (unsigned)n0); w32(b + 52 + 16, (unsigned)(o - n0));
  return o;
}

int main(int argc, char **argv) {
  unsigned char b[1024];
  char nome[17];
  int sinal;
  size_t n = core32(b, 11, "nuvio-proto");
  assert(coredump_diag_notas(b, n, nome, &sinal) == 1);
  assert(!strcmp(nome, "nuvio-proto") && sinal == 11);
  /* Cut before the notes: nothing claimed. */
  assert(coredump_diag_notas(b, 60, nome, &sinal) == 0 && !nome[0] && !sinal);
  /* Not a core. */
  b[16] = 2;
  assert(coredump_diag_notas(b, n, nome, &sinal) == 0);
  assert(coredump_diag_notas((const unsigned char *)"hello", 5, nome, &sinal) == 0);
  /* A real core of this machine (64-bit), when one is given. */
  if (argc > 2) {
    FILE *f = fopen(argv[1], "rb");
    static unsigned char big[256 * 1024];
    size_t k = f ? fread(big, 1, sizeof big, f) : 0;
    if (f) fclose(f);
    assert(coredump_diag_notas(big, k, nome, &sinal) == 1);
    printf("real core: process=%s signal=%d\n", nome, sinal);
    assert(!strcmp(nome, argv[2]) && sinal == 11);
  }
  /* The startup report and the exit markers, on this machine. */
  if (argc > 3 && !chdir(argv[3])) {
    coredump_diag_relatar();
    coredump_diag_armar_saida();
    coredump_diag_saida("main loop left");
    coredump_diag_armar_saida();
    coredump_diag_saida("main returns");
  }
  puts("coredump_diag: ok");
  return 0;
}
