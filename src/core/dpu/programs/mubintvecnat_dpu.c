#include <stddef.h>
#include <mram.h>
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <alloc.h>
#include <perfcounter.h>
#include <alloc.h>
#include "helper_functions.h"

#define CACHE_SIZE (1 << 11)

__host uint64_t mram_modulus;
__host uint32_t nb_cycles;
__host uint64_t data_copied_in_bytes;

int main() {
    perfcounter_config(COUNT_CYCLES, true);

    unsigned int tasklet_id = me();
    if (tasklet_id == 0) {
        mem_reset();  // Resets the heap
    }
    uint64_t* cache_A = (uint64_t*)mem_alloc(CACHE_SIZE);
    uint64_t* cache_B = (uint64_t*)mem_alloc(CACHE_SIZE);

    uint64_t mram_base_addr_A = (uint64_t)DPU_MRAM_HEAP_POINTER;
    uint64_t mram_base_addr_B = (uint64_t)(DPU_MRAM_HEAP_POINTER + data_copied_in_bytes);

    for (unsigned int bytes_index = tasklet_id * CACHE_SIZE; bytes_index < data_copied_in_bytes;
         bytes_index += CACHE_SIZE * NR_TASKLETS) {
        mram_read((__mram_ptr void const*)(mram_base_addr_A + bytes_index), cache_A, CACHE_SIZE);
        mram_read((__mram_ptr void const*)(mram_base_addr_B + bytes_index), cache_B, CACHE_SIZE);

        ModAddEq(cache_A, cache_B, mram_modulus, CACHE_SIZE / sizeof(uint64_t));

        mram_write(cache_A, (__mram_ptr void*)(mram_base_addr_A + bytes_index), CACHE_SIZE);
    }
    printf("\nDone adding in the DPU\n");

    nb_cycles = perfcounter_get();
}