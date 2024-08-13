#include "../include/common.h"
#include "../include/element-wise-ops.h"
#include <barrier.h>
#include <stdint.h>
#include <stdio.h>

__host struct mod_add_sub_mult meta;

BARRIER_INIT(my_barrier, NR_TASKLETS);

int scalar(void);
int scalar_eq(void);
int vector(void);
int vector_eq(void);

int (*kernels[])(void) = {scalar, scalar_eq, vector, vector_eq};

int main(void) {
  // Kernel
  return kernels[meta.kernel]();
}

int scalar() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op1 = meta.op1.start;
  uint64_t op2 = meta.op2.start;
  uint32_t res = meta.res.start;
  uint32_t op1_size = meta.op1.size;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op1_size ? CACHE_SIZE : op1_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op1);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op1_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);
    // mram_read((__mram_ptr void const *)(mram_base_addr_B + bytes_index),
    //           cache_B, size_to_copy_bytes);
    // Need to work on the structure to pass the scalar value
    modular_subtraction_scalar(cache_A, op2, result, meta.mod, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", result[i]);
    }
    printf("\n");

    mram_write(result, (__mram_ptr void *)(mram_base_addr_res + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int scalar_eq() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op1 = meta.op1.start;
  uint64_t op2 = meta.op2.start;
  uint32_t op1_size = meta.op1.size;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op1_size ? CACHE_SIZE : op1_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op1);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op1_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);
    // mram_read((__mram_ptr void const *)(mram_base_addr_B + bytes_index),
    //           cache_B, size_to_copy_bytes);

    // Need to work on the structure to pass the scalar value.
    modular_subtratcion_scalar_Eq(cache_A, op2, meta.mod, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");

    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int vector() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op1 = meta.op1.start;
  uint32_t op2 = meta.op2.start;
  uint32_t res = meta.res.start;
  uint32_t op1_size = meta.op1.size;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op1_size ? CACHE_SIZE : op1_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op1);
  uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op1_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);
    mram_read((__mram_ptr void const *)(mram_base_addr_B + bytes_index),
              cache_B, size_to_copy_bytes);

    modular_subtraction_vector(cache_A, cache_B, result, meta.mod,
                               size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", result[i]);
    }
    printf("\n");

    mram_write(result, (__mram_ptr void *)(mram_base_addr_res + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int vector_eq() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op1 = meta.op1.start;
  uint32_t op2 = meta.op2.start;
  uint32_t op1_size = meta.op1.size;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op1_size ? CACHE_SIZE : op1_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op1);
  uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op1_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);
    mram_read((__mram_ptr void const *)(mram_base_addr_B + bytes_index),
              cache_B, size_to_copy_bytes);

    modular_subtraction_vector_Eq(cache_A, cache_B, meta.mod, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");

    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}
