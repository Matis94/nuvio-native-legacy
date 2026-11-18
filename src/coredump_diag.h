#ifndef NV_COREDUMP_DIAG_H
#define NV_COREDUMP_DIAG_H
#include <stddef.h>
/* TEMPORARY DIAGNOSTICS (issue: core dump files filling the TV's storage).
 * At start: the core dump setup (RLIMIT_CORE, core_pattern, working folder),
 * every core file left in the app's folders (size, time, which process and
 * signal, read from its ELF notes) and the exit trace of the previous session.
 * "[saida]" and "[coredump-net]" share monotonic milliseconds and Linux
 * thread IDs, so the previous trace can be matched to the crash report. */

/* Logs what is above; call once the log file is open. */
void coredump_diag_relatar(void);
/* One "[saida] <step>" line with ms since the first marker of this exit. */
void coredump_diag_saida(const char *etapa);
/* Registers one marker early in main() and the other just before it returns. */
void coredump_diag_armar_saida(void);
/* Reports entry/return of a curl operation without logging request data. */
void coredump_diag_rede(const char *operacao, int inicio, int resultado);

/* From the start of a core file: the process name (NT_PRPSINFO pr_fname) and
 * the signal (NT_PRSTATUS si_signo). ELF32 (arm) and ELF64 (aarch64) cores.
 * Returns 1 when the notes were found. Exposed for the test. */
int coredump_diag_notas(const unsigned char *b, size_t n, char nome[17], int *sinal);
#endif
