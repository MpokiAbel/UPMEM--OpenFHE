#include <stddef.h>
#include <mram.h>
#include <stdio.h>
#include <defs.h>
#include <alloc.h>
#include "helper_functions.h"

/*
    We represent a buffer to have a size of 64 KB and handle the computations on 64 bits basis after.
    The cache maximum number of bytes is 2048 (2KB) and we initialize it with that.
*/

#define BUFFER_SIZE (1 << 16)
#define CACHE_SIZE  256

__mram_noinit uint8_t mram_array_1[BUFFER_SIZE];
__mram_noinit uint8_t mram_array_2[BUFFER_SIZE];
__dma_aligned __host uint64_t mram_modulus;

int main() {
    __dma_aligned uint8_t local_array_1[CACHE_SIZE];
    __dma_aligned uint8_t local_array_2[CACHE_SIZE];

    for (unsigned int bytes_index = me() * CACHE_SIZE; bytes_index < BUFFER_SIZE;
         bytes_index += CACHE_SIZE * NR_TASKLETS) {
        mram_read(&mram_array_1[bytes_index], local_array_1, CACHE_SIZE);
        mram_read(&mram_array_2[bytes_index], local_array_2, CACHE_SIZE);

        ModAddEq((uint64_t*)local_array_1, (uint64_t*)local_array_2, mram_modulus, CACHE_SIZE / sizeof(uint64_t));

        mram_write(local_array_1, &mram_array_1[bytes_index], CACHE_SIZE);

        // printf("\nTasklet %u %d \n", me(), NR_TASKLETS);
    }

    printf("\nDone adding in the DPU\n");
    mem_reset();
}