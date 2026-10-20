// Settings "Tamanho do buffer" -> MiB for the next open, with the free-RAM clamp.
#include "../src/cacheboost.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  // Auto stays Auto (backend default), whatever the memory.
  assert(cacheboost_buffer_mb(0, 4096) == 0);
  assert(cacheboost_buffer_mb(0, -1) == 0);
  // Plenty of RAM: the option as asked.
  assert(cacheboost_buffer_mb(1, 4096) == 64);
  assert(cacheboost_buffer_mb(2, 4096) == 128);
  assert(cacheboost_buffer_mb(3, 4096) == 256);
  // LG C5 report: ~500 MiB free before the buffer -> 256 fits above the floor.
  assert(cacheboost_buffer_mb(3, 500) == 256);
  // DTS allocates ceil(MiB / 4) queued ranges plus one active range.
  // At 448 MiB free, 64 queued ranges plus the active cache break the floor.
  const long available[] = {255, 256, 259, 260, 319, 320, 323, 324, 447, 448, 451, 452, 500};
  for (size_t i = 0; i < sizeof available / sizeof available[0]; i++) {
    long freeMb = available[i];
    for (int option = 1; option <= 3; option++) {
      int mb = cacheboost_buffer_mb(option, freeMb);
      if (!mb) continue; // Auto preserves the backend default.
      long backendBytes = ((mb + 3) / 4 + 1L) * (4L * 1024 * 1024 - 1);
      assert(freeMb * 1024 * 1024 - backendBytes >= CB_BUFFER_PISO_MB * 1024L * 1024);
    }
  }
  // Tight RAM steps down the ladder, never below the floor.
  assert(cacheboost_buffer_mb(3, 448) == 128);
  assert(cacheboost_buffer_mb(3, 451) == 128);
  assert(cacheboost_buffer_mb(3, 452) == 256);
  assert(cacheboost_buffer_mb(2, 324) == 128);
  assert(cacheboost_buffer_mb(1, 260) == 64);
  assert(cacheboost_buffer_mb(3, 447) == 128);
  assert(cacheboost_buffer_mb(3, 320) == 64);
  assert(cacheboost_buffer_mb(3, 300) == 64);
  assert(cacheboost_buffer_mb(2, 255) == 0);
  assert(cacheboost_buffer_mb(1, 256) == 0);
  // Unknown memory or out-of-range option -> Auto.
  assert(cacheboost_buffer_mb(3, -1) == 0);
  assert(cacheboost_buffer_mb(4, 4096) == 0 && cacheboost_buffer_mb(-1, 4096) == 0);
  // The setting is queued read-ahead. Measured DTS/DV capacity also includes
  // its active range, and the official RAM clamp can lower the queue.
  const long long MiB = 1048576;
  char text[96];
  cacheboost_buffer_texto(text, sizeof text, 250*MiB, 260*MiB-65, 3);
  assert(!strcmp(text, "Buffer: 250.0 / 260 MiB total (256 MiB selected)"));
  cacheboost_buffer_texto(text, sizeof text, 120*MiB, 132*MiB-33, 3);
  assert(!strcmp(text, "Buffer: 120.0 / 132 MiB total (256 MiB selected)"));
  cacheboost_buffer_texto(text, sizeof text, 50*MiB, 68*MiB-17, 1);
  assert(strstr(text, "68 MiB total (64 MiB selected)"));
  cacheboost_buffer_texto(text, sizeof text, 250*MiB, 256*MiB, 3);
  assert(!strcmp(text, "Buffer: 250.0 / 256 MiB total"));
  cacheboost_buffer_texto(text, sizeof text, 10*MiB, 33*MiB, 0);
  assert(!strcmp(text, "Buffer: 10.0 / 33 MiB total"));
  cacheboost_buffer_texto(text, sizeof text, 0, 0, 3);
  assert(!strcmp(text, "Buffer: system"));
  cacheboost_buffer_texto(text, 1, 0, MiB, 3); assert(!text[0]);
  cacheboost_buffer_texto(NULL, 0, 0, 0, 0);
  puts("buffer_tamanho: ok");
  return 0;
}
