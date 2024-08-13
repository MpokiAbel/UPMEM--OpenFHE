#include "../include/element-wise-ops.h"
#include <stdio.h>

int main() {
  unsigned int tasklet_id = me();

  if (tasklet_id == 0) {
    // printf("PIM started !!\n");
    mem_reset(); // Resets the heap
  }
  NativeInt *cache_A = (NativeInt *)mem_alloc(CACHE_SIZE);
  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + 0);

  mram_read((__mram_ptr void const *)mram_base_addr_A, cache_A, 64);

  if (tasklet_id == 0) {
    for (int i = 0; i < 8; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");
  }
  return 0;
}
