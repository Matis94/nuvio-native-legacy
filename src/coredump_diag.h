#ifndef NV_COREDUMP_DIAG_H
#define NV_COREDUMP_DIAG_H
#include <stddef.h>
/* TEMPORARY DIAGNOSTICS (issue: core dump files filling the TV's storage).
 * At start: the core dump setup (RLIMIT_CORE, core_pattern, working folder),
 * every core file left in the app's folders (size, time, which process and
 * signal, read from its ELF notes) and the exit trace of the previous session.
 * On the way out: "[saida]" markers, written with write() only, so the last
 * step reached before a crash after main() can be told. */

/* Logs what is above; call once the log file is open. */
void coredump_diag_relatar(void);
/* One "[saida] <step>" line with ms since the first marker of this exit. */
void coredump_diag_saida(const char *etapa);
/* Registers the atexit markers (the first and the last handler to run). */
void coredump_diag_armar_saida(void);

/* From the start of a core file: the process name (NT_PRPSINFO pr_fname) and
 * the signal (NT_PRSTATUS si_signo). ELF32 (arm) and ELF64 (aarch64) cores.
 * Returns 1 when the notes were found. Exposed for the test. */
int coredump_diag_notas(const unsigned char *b, size_t n, char nome[17], int *sinal);
#endif
