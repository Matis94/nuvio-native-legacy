#ifndef NV_MEMLOG_H
#define NV_MEMLOG_H
#include <stddef.h>

typedef struct {
  long rss_kib, peak_kib, available_kib, total_kib;
} MemlogInfo;

/* -1 means unavailable; RSS excludes some driver/GPU/system allocations. */
void memlog_ler(MemlogInfo *info);

// So nos pontos de log, nunca por quadro. Sem /proc valido, sufixo vazio.
void memlog_amostra(char *sufixo, size_t tamanho);
void memlog_evento(const char *evento);
int memlog_pouca_memoria(void);
#endif
