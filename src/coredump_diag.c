// TEMPORARY DIAGNOSTICS for core dump files piling up in the app's folder on
// LG TVs (reported on a C1: ~1.9 GB of core.* files, no visible crash, a new
// one appearing while the TV was off). See coredump_diag.h.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE   /* realpath, PATH_MAX and readlink under -std=c11 */
#endif
#include "coredump_diag.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static uint32_t rd16(const unsigned char *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t rd32(const unsigned char *p) { return rd16(p) | rd16(p + 2) << 16; }
static uint64_t rd64(const unsigned char *p) { return (uint64_t)rd32(p) | (uint64_t)rd32(p + 4) << 32; }

static int nomeValido(const unsigned char *p, char nome[17]) {
  int i;
  for (i = 0; i < 16 && p[i]; i++) if (p[i] < 0x20 || p[i] > 0x7e) return 0;
  if (i == 0) return 0;
  memcpy(nome, p, (size_t)i); nome[i] = 0;
  return 1;
}

int coredump_diag_notas(const unsigned char *b, size_t n, char nome[17], int *sinal) {
  int e64, achou = 0;
  uint64_t phoff;
  uint32_t phent, phnum, i;
  nome[0] = 0; *sinal = 0;
  if (!b || n < 52 || memcmp(b, "\x7f" "ELF", 4) || b[5] != 1 /* little endian */) return 0;
  e64 = b[4] == 2;
  if (rd16(b + 16) != 4 /* ET_CORE */) return 0;
  if (e64) { if (n < 64) return 0; phoff = rd64(b + 32); phent = rd16(b + 54); phnum = rd16(b + 56); }
  else { phoff = rd32(b + 28); phent = rd16(b + 42); phnum = rd16(b + 44); }
  if (phent < (e64 ? 56u : 32u)) return 0;
  for (i = 0; i < phnum && i < 4096; i++) {
    uint64_t ph = phoff + (uint64_t)i * phent, off, sz, o;
    if (ph + phent > n) break;
    if (rd32(b + ph) != 4 /* PT_NOTE */) continue;
    off = e64 ? rd64(b + ph + 8) : rd32(b + ph + 4);
    sz = e64 ? rd64(b + ph + 32) : rd32(b + ph + 16);
    if (off >= n) continue;
    if (sz > n - off) sz = n - off;
    for (o = 0; o + 12 <= sz;) {
      const unsigned char *h = b + off + o;
      uint32_t nsz = rd32(h), dsz = rd32(h + 4), tipo = rd32(h + 8);
      uint64_t d = o + 12 + ((nsz + 3u) & ~3u);
      if (d + dsz > sz) break;
      const unsigned char *desc = b + off + d;
      if (tipo == 1 /* NT_PRSTATUS */ && dsz >= 14 && !*sinal) {
        int s = (int)(int16_t)rd16(desc + 12);   // pr_cursig
        *sinal = s > 0 ? s : (int)rd32(desc);    // elf_siginfo.si_signo
        achou = 1;
      } else if (tipo == 3 /* NT_PRPSINFO */ && !nome[0]) {
        // pr_fname: offset 40 in the 64-bit layout; 28 for 32-bit ARM (16-bit
        // uid/gid, also the arm64 compat layout), 32 where uid/gid are 32-bit.
        static const unsigned o64[] = { 40 }, o32[] = { 28, 32 };
        const unsigned *cand = e64 ? o64 : o32;
        int nc = e64 ? 1 : 2;
        for (int k = 0; k < nc && !nome[0]; k++)
          if (dsz >= cand[k] + 16) nomeValido(desc + cand[k], nome);
        achou = 1;
      }
      o = d + ((dsz + 3u) & ~3u);
    }
  }
  return achou;
}

#if defined(__linux__) && !defined(NV_TPK) && !defined(NV_ANDROID) && !defined(__EMSCRIPTEN__)
#include "dados.h"
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/prctl.h>

#define CD_LER_MAX   (256 * 1024)
#define CD_POR_PASTA 15

static void limite(char *s, size_t tam, rlim_t v) {
  if (v == RLIM_INFINITY) snprintf(s, tam, "unlimited");
  else snprintf(s, tam, "%llu", (unsigned long long)v);
}

static void lerLinha(const char *arq, char *s, size_t tam) {
  FILE *f = fopen(arq, "r");
  s[0] = 0;
  if (!f) { snprintf(s, tam, "(unreadable)"); return; }
  if (!fgets(s, (int)tam, f)) s[0] = 0;
  fclose(f);
  s[strcspn(s, "\n")] = 0;
}

static int ehCore(const char *nome) {
  return !strcmp(nome, "core") || !strncmp(nome, "core.", 5) || !strncmp(nome, "core-", 5);
}

static void infoCore(const char *caminho, char *out, size_t tam) {
  int fd = open(caminho, O_RDONLY);
  unsigned char *b;
  ssize_t n;
  char nome[17];
  int sinal;
  snprintf(out, tam, "not readable");
  if (fd < 0) return;
  b = malloc(CD_LER_MAX);
  if (b && (n = read(fd, b, CD_LER_MAX)) > 0) {
    if (coredump_diag_notas(b, (size_t)n, nome, &sinal))
      snprintf(out, tam, "process=%s signal=%d", nome[0] ? nome : "?", sinal);
    else snprintf(out, tam, "no ELF core notes in the first %ld bytes", (long)n);
  }
  free(b);
  close(fd);
}

static int jaVista(char vistas[][PATH_MAX], int n, const char *r) {
  for (int i = 0; i < n; i++) if (!strcmp(vistas[i], r)) return 1;
  return 0;
}

static void varrer(const char *pasta, const char *porque, char vistas[][PATH_MAX], int *nv) {
  char real[PATH_MAX];
  DIR *d;
  struct dirent *e;
  int achados = 0;
  unsigned long long total = 0;
  time_t agora = time(NULL);
  if (!pasta || !pasta[0] || !realpath(pasta, real) || jaVista(vistas, *nv, real) || *nv >= 8) return;
  snprintf(vistas[(*nv)++], PATH_MAX, "%s", real);
  if (!(d = opendir(real))) { printf("[coredump] %s (%s): can't list\n", real, porque); return; }
  while ((e = readdir(d))) {
    char c[PATH_MAX + 260], quando[32], info[96];
    struct stat st;
    if (!ehCore(e->d_name)) continue;
    snprintf(c, sizeof c, "%s/%s", real, e->d_name);
    if (stat(c, &st) || !S_ISREG(st.st_mode)) continue;
    achados++; total += (unsigned long long)st.st_size;
    if (achados > CD_POR_PASTA) continue;
    strftime(quando, sizeof quando, "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
    infoCore(c, info, sizeof info);
    printf("[coredump]   %s: %.1f MB, %s (%ld min ago), uid=%u, %s\n", e->d_name, st.st_size / 1048576.0,
           quando, (long)((agora - st.st_mtime) / 60), (unsigned)st.st_uid, info);
  }
  closedir(d);
  printf("[coredump] %s (%s): %d core file(s), %.1f MB%s\n", real, porque, achados, total / 1048576.0,
         achados > CD_POR_PASTA ? " (first ones listed)" : "");
}

// The previous session's exit trace: its "[saida]"/"[queda]" lines and its
// last lines, from the tail of the log main() kept from it.
static void saidaAnterior(void) {
  const char *arq = getenv("NUVIO_LOG_ANTERIOR");
  FILE *f;
  long tam, ini;
  char *b, *l, *ult[3] = { 0 };
  size_t n;
  int linhas = 0;
  if (!arq || !arq[0] || !(f = fopen(arq, "r"))) { printf("[coredump] previous log: none\n"); return; }
  fseek(f, 0, SEEK_END); tam = ftell(f);
  ini = tam > 65536 ? tam - 65536 : 0;
  fseek(f, ini, SEEK_SET);
  b = malloc(65537);
  n = b ? fread(b, 1, 65536, f) : 0;
  fclose(f);
  if (!b) return;
  b[n] = 0;
  printf("[coredump] previous log: %ld bytes; its exit trace:\n", tam);
  for (l = strtok(b, "\n"); l; l = strtok(NULL, "\n")) {
    if (strstr(l, "[saida]") || strstr(l, "[queda]") || strstr(l, "SIGTERM") || strstr(l, "SIGALRM")) {
      printf("[coredump]   < %s\n", l); linhas++;
    }
    ult[0] = ult[1]; ult[1] = ult[2]; ult[2] = l;
  }
  if (!linhas) printf("[coredump]   < (no exit markers: an older build, or it ended before reaching them)\n");
  for (int i = 0; i < 3; i++) if (ult[i]) printf("[coredump]   last: %s\n", ult[i]);
  free(b);
}

void coredump_diag_relatar(void) {
  struct rlimit rl;
  char cur[24] = "?", max[24] = "?", pat[256], pid[16], cwd[PATH_MAX] = "?", exe[PATH_MAX] = "", padrao[PATH_MAX] = "";
  static char vistas[8][PATH_MAX];
  int nv = 0;
  ssize_t k;
  if (!getrlimit(RLIMIT_CORE, &rl)) { limite(cur, sizeof cur, rl.rlim_cur); limite(max, sizeof max, rl.rlim_max); }
  lerLinha("/proc/sys/kernel/core_pattern", pat, sizeof pat);
  lerLinha("/proc/sys/kernel/core_uses_pid", pid, sizeof pid);
  if (!getcwd(cwd, sizeof cwd)) snprintf(cwd, sizeof cwd, "?");
  if ((k = readlink("/proc/self/exe", exe, sizeof exe - 1)) > 0) {
    exe[k] = 0;
    char *bar = strrchr(exe, '/');
    if (bar) *bar = 0;
  } else exe[0] = 0;
  printf("[coredump] RLIMIT_CORE soft=%s hard=%s dumpable=%d core_pattern=\"%s\" core_uses_pid=%s cwd=%s\n",
         cur, max, prctl(PR_GET_DUMPABLE, 0, 0, 0, 0), pat, pid, cwd);
  // An absolute core_pattern writes to its own folder; a relative one to the cwd.
  if (pat[0] == '/') {
    snprintf(padrao, sizeof padrao, "%s", pat);
    char *bar = strrchr(padrao, '/');
    if (bar && bar != padrao) *bar = 0;
  }
  varrer(cwd, "cwd", vistas, &nv);
  varrer(exe, "app folder", vistas, &nv);
  varrer(dados_dir(), "data folder", vistas, &nv);
  varrer(padrao, "core_pattern folder", vistas, &nv);
  varrer("/tmp", "tmp", vistas, &nv);
  saidaAnterior();
  fflush(stdout);
}

static struct timespec saidaT0;
static int saidaIniciada;

void coredump_diag_saida(const char *etapa) {
  struct timespec t;
  char b[160];
  int n;
  long ms;
  clock_gettime(CLOCK_MONOTONIC, &t);
  if (!saidaIniciada) { saidaT0 = t; saidaIniciada = 1; }
  ms = (long)((t.tv_sec - saidaT0.tv_sec) * 1000 + (t.tv_nsec - saidaT0.tv_nsec) / 1000000);
  n = snprintf(b, sizeof b, "[saida] %s (+%ld ms)\n", etapa ? etapa : "?", ms);
  if (n > 0) { fflush(stdout); (void)!write(1, b, (size_t)(n < (int)sizeof b ? n : (int)sizeof b - 1)); }
}

// Registered at start: runs LAST, after the libraries' own exit handlers.
static void ultimoAtexit(void) { coredump_diag_saida("atexit: last handler (libraries' cleanup done)"); }
// Registered at the end of main: runs FIRST.
static void primeiroAtexit(void) { coredump_diag_saida("atexit: first handler (libraries' cleanup next)"); }

void coredump_diag_armar_saida(void) {
  static int armado;
  if (!armado) { atexit(ultimoAtexit); armado = 1; }
  else atexit(primeiroAtexit);
}
#else
void coredump_diag_relatar(void) {}
void coredump_diag_saida(const char *etapa) { (void)etapa; }
void coredump_diag_armar_saida(void) {}
#endif
