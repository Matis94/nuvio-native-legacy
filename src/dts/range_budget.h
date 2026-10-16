#ifndef NV_DTS_RANGE_BUDGET_H
#define NV_DTS_RANGE_BUDGET_H

/* Test build: compressed HTTP payload only, not total player/process memory.
 * Reserve one full range for the active consumer cache. The remaining slots
 * include in-flight downloads as well as completed prefetches. */
#define NV_RANGE_BUDGET_MIB 64
#define NV_RANGE_MAX_MIB 256
#define NV_RANGE_BYTES (1024 * 1024)
#define NV_RANGE_BYTES_DV (4 * 1024 * 1024 - 1)
#define NV_RANGE_SLOTS (NV_RANGE_MAX_MIB - 1)
#define NV_RANGE_SLOTS_DV (NV_RANGE_BUDGET_MIB / 4 - 1)

_Static_assert((NV_RANGE_SLOTS + 1ULL) * NV_RANGE_BYTES <=
               NV_RANGE_MAX_MIB * 1024ULL * 1024ULL, "HTTP payload budget");
_Static_assert((NV_RANGE_SLOTS_DV + 1ULL) * NV_RANGE_BYTES_DV <=
               NV_RANGE_BUDGET_MIB * 1024ULL * 1024ULL, "DV HTTP payload budget");
_Static_assert(NV_RANGE_SLOTS_DV <= NV_RANGE_SLOTS, "DV slots fit range array");
#endif
