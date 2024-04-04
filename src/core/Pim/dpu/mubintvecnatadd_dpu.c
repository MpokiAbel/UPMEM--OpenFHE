#include <stddef.h>
#include <mram.h>
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <alloc.h>
#include <perfcounter.h>
#include <alloc.h>
#include "intnat-dpu.h"

#define CACHE_SIZE (1 << 8)
#define SIZE (CACHE_SIZE >> 3)

__host uint64_t mram_modulus;
__host uint64_t data_copied_in_bytes;

int main() {
    // perfcounter_config(COUNT_CYCLES, true);
    unsigned int tasklet_id = me();
    if (tasklet_id == 0) {
        mem_reset();  // Resets the heap
    }
    NativeInt* cache_A = (NativeInt*)mem_alloc(CACHE_SIZE);
    NativeInt* cache_B = (NativeInt*)mem_alloc(CACHE_SIZE);

    uint32_t mram_base_addr_A = (uint32_t)DPU_MRAM_HEAP_POINTER;
    uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + data_copied_in_bytes);

    for (unsigned int bytes_index = tasklet_id * CACHE_SIZE; bytes_index < data_copied_in_bytes;
         bytes_index += CACHE_SIZE * NR_TASKLETS) {
        mram_read((__mram_ptr void const*)(mram_base_addr_A + bytes_index), cache_A, CACHE_SIZE);
        mram_read((__mram_ptr void const*)(mram_base_addr_B + bytes_index), cache_B, CACHE_SIZE);

        ModAddEq(cache_A, cache_B, mram_modulus, SIZE);

        mram_write(cache_A, (__mram_ptr void*)(mram_base_addr_A + bytes_index), CACHE_SIZE);
    }

    // nb_cycles = perfcounter_get();
}