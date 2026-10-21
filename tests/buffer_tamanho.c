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
  // Official da7b893 floor: reserve 100 MiB after queue + active 4 MiB.
  assert(CB_BUFFER_PISO_MB == 100);
  for (long freeMb = 0; freeMb <= 500; freeMb++) {
    for (int option = 1; option <= 3; option++) {
      int mb = cacheboost_buffer_mb(option, freeMb);
      assert(!mb || freeMb - mb - 4 >= CB_BUFFER_PISO_MB);
    }
  }
  assert(cacheboost_buffer_mb(3, 360) == 256);
  assert(cacheboost_buffer_mb(3, 359) == 128);
  assert(cacheboost_buffer_mb(2, 232) == 128);
  assert(cacheboost_buffer_mb(2, 231) == 64);
  assert(cacheboost_buffer_mb(1, 168) == 64);
  assert(cacheboost_buffer_mb(1, 167) == 0);
  assert(cacheboost_buffer_mb(3, 300) == 128);
  assert(cacheboost_buffer_mb(3, 250) == 128);
  assert(cacheboost_buffer_mb(3, 150) == 0);
  // Unknown memory or out-of-range option -> Auto.
  assert(cacheboost_buffer_mb(3, -1) == 0);
  assert(cacheboost_buffer_mb(4, 4096) == 0 && cacheboost_buffer_mb(-1, 4096) == 0);
  // Explicit RAM opt-out respects supported choices even with low/unknown RAM.
  const long tight[] = {-1, 0, 99, 100, 150, 167, 168, 231, 232, 359, 360};
  for (size_t i = 0; i < sizeof tight / sizeof tight[0]; i++) {
    for (int option = -1; option <= 4; option++) {
      int requested = option >= 1 && option <= 3 ? 32 << option : 0;
      assert(cacheboost_buffer_mb_policy(option, tight[i], 0) == requested);
      assert(cacheboost_buffer_mb_policy(option, tight[i], 1) == cacheboost_buffer_mb(option, tight[i]));
    }
  }
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
