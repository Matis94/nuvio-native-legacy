#ifndef NV_COREGUARDA_H
#define NV_COREGUARDA_H
/* Keeps crash dump files (core, core.<pid>) from filling the TV's storage on
 * webOS. Some TVs (LG C1, webOS 6) write each dump as a file of about the
 * app's memory size into its working folder; a few of them reached ~1.9 GB.
 * No-ops on every other target. */

/* Sets the soft RLIMIT_CORE to 0, so the kernel writes no core file for this
 * process. Call at the top of main(), before any thread starts. A crash is
 * still logged by the app's own report ([queda]). */
void coreguarda_bloquear(void);

/* Removes crash dump files left by earlier sessions in the working folder, the
 * app folder and the data folder: regular files named "core" or
 * "core.<digits>", owned by this user, that start with an ELF core header.
 * Logs one "[coreguard]" line. Call after the log is open and dados_iniciar. */
void coreguarda_limpar(void);

/* Ends the process with _exit(0) after flushing stdio, skipping atexit
 * handlers and library destructors. OpenSSL's exit cleanup freed data that a
 * detached network thread was still using (SIGSEGV in libcrypto after main
 * returned). Call last, once everything has been saved. */
void coreguarda_sair(void)
#if defined(__GNUC__)
  __attribute__((noreturn))
#endif
  ;

/* For the test: 1 when the file is a crash dump coreguarda_limpar removes. */
int coreguarda_eh_dump(const char *pasta, const char *nome);
/* For the test: removes dumps in one folder; returns how many. */
int coreguarda_limpar_pasta(const char *pasta, unsigned long long *bytes);
#endif
