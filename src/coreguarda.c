// Crash dump files filling the TV's storage on webOS: see coreguarda.h.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE   /* realpath, PATH_MAX and readlink under -std=c11 */
#endif
#include "coreguarda.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__) && !defined(__EMSCRIPTEN__) && !defined(NV_TPK) && !defined(NV_ANDROID)
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

static int nomeDeDump(const char *nome) {
  const char *p;
  if (!strcmp(nome, "core")) return 1;
  if (strncmp(nome, "core.", 5) || !nome[5]) return 0;
  for (p = nome + 5; *p; p++) if (*p < '0' || *p > '9') return 0;
  return 1;
}

int coreguarda_eh_dump(const char *pasta, const char *nome) {
  char c[PATH_MAX];
  unsigned char h[18];
  struct stat st;
  int fd, ok = 0;
  if (!pasta || !nome || !nomeDeDump(nome)) return 0;
  if (snprintf(c, sizeof c, "%s/%s", pasta, nome) >= (int)sizeof c) return 0;
  if (lstat(c, &st) || !S_ISREG(st.st_mode) || st.st_uid != getuid()) return 0;
  if ((fd = open(c, O_RDONLY | O_NOFOLLOW)) < 0) return 0;
  // ELF magic, then e_type (offset 16, little endian) == ET_CORE.
  if (read(fd, h, sizeof h) == (ssize_t)sizeof h && !memcmp(h, "\x7f" "ELF", 4) &&
      h[5] == 1 && h[16] == 4 && h[17] == 0)
    ok = 1;
  close(fd);
  return ok;
}

int coreguarda_limpar_pasta(const char *pasta, unsigned long long *bytes) {
  DIR *d;
  struct dirent *e;
  int n = 0;
  if (!pasta || !pasta[0] || !(d = opendir(pasta))) return 0;
  while ((e = readdir(d))) {
    char c[PATH_MAX];
    struct stat st;
    if (!coreguarda_eh_dump(pasta, e->d_name)) continue;
    if (snprintf(c, sizeof c, "%s/%s", pasta, e->d_name) >= (int)sizeof c) continue;
    if (lstat(c, &st)) continue;
    if (!unlink(c)) { n++; if (bytes) *bytes += (unsigned long long)st.st_size; }
  }
  closedir(d);
  return n;
}
#else
int coreguarda_eh_dump(const char *pasta, const char *nome) { (void)pasta; (void)nome; return 0; }
int coreguarda_limpar_pasta(const char *pasta, unsigned long long *bytes) {
  (void)pasta; (void)bytes; return 0;
}
#endif

#if defined(NV_WEBOS) && defined(__linux__)
#include "dados.h"

static char limiteAntes[64] = "?";

static void fmtLimite(char *s, size_t tam, rlim_t v) {
  if (v == RLIM_INFINITY) snprintf(s, tam, "unlimited");
  else snprintf(s, tam, "%llu", (unsigned long long)v);
}

void coreguarda_bloquear(void) {
  struct rlimit rl;
  char cur[24], max[24];
  if (getrlimit(RLIMIT_CORE, &rl)) return;
  fmtLimite(cur, sizeof cur, rl.rlim_cur);
  fmtLimite(max, sizeof max, rl.rlim_max);
  snprintf(limiteAntes, sizeof limiteAntes, "soft=%s hard=%s", cur, max);
  rl.rlim_cur = 0;
  (void)setrlimit(RLIMIT_CORE, &rl);
}

void coreguarda_limpar(void) {
  char vistas[3][PATH_MAX], cwd[PATH_MAX], exe[PATH_MAX], real[PATH_MAX];
  const char *pastas[3];
  struct rlimit rl;
  unsigned long long bytes = 0;
  int nv = 0, n = 0;
  ssize_t k;
  pastas[0] = getcwd(cwd, sizeof cwd) ? cwd : NULL;
  if ((k = readlink("/proc/self/exe", exe, sizeof exe - 1)) > 0) {
    char *bar;
    exe[k] = 0;
    if ((bar = strrchr(exe, '/'))) *bar = 0;
    pastas[1] = exe;
  } else pastas[1] = NULL;
  pastas[2] = dados_dir();
  for (int i = 0; i < 3; i++) {
    int dup = 0;
    if (!pastas[i] || !pastas[i][0] || !realpath(pastas[i], real)) continue;
    for (int j = 0; j < nv; j++) if (!strcmp(vistas[j], real)) dup = 1;
    if (dup) continue;
    snprintf(vistas[nv++], PATH_MAX, "%s", real);
    n += coreguarda_limpar_pasta(real, &bytes);
  }
  if (getrlimit(RLIMIT_CORE, &rl)) rl.rlim_cur = RLIM_INFINITY;
  printf("[coreguard] core files %s (RLIMIT_CORE was %s); removed %d old crash dump file(s), %.1f MB\n",
         rl.rlim_cur == 0 ? "disabled" : "NOT disabled", limiteAntes, n, bytes / 1048576.0);
  fflush(stdout);
}

void coreguarda_sair(void) {
  fflush(NULL);
  _exit(0);
}
#else
void coreguarda_bloquear(void) {}
void coreguarda_limpar(void) {}
void coreguarda_sair(void) { exit(0); }
#endif
