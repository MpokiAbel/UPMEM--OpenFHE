#include "../include/common.h"
#include "../include/element-wise-ops.h"
#include <barrier.h>
#include <stdint.h>
#include <stdio.h>

__host struct mod_ops meta;

BARRIER_INIT(my_barrier, NR_TASKLETS);

int SwitchModulus(void);
int MOD(void);
int MODEq(void);
int MODByTwo(void);
int MODByTwoEq(void);
int MODexp(void);
int MODexpEq(void);
int MODInverse(void);
int MODInverseEq(void);
int MULTIPLYAndRound(void);
int MULTIPLYAndRoundEq(void);
int DIVIDEAndRound(void);
int DIVIDEAndRoundEq(void);

int (*kernels[])(void) = {SwitchModulus,
                          MOD,
                          MODEq,
                          MODByTwo,
                          MODByTwoEq,
                          MODexp,
                          MODexpEq,
                          MODInverse,
                          MODInverseEq,
                          MULTIPLYAndRound,
                          MULTIPLYAndRoundEq,
                          DIVIDEAndRound,
                          DIVIDEAndRoundEq};

int main(void) { return kernels[meta.kernel](); }

int SwitchModulus() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint64_t om = meta.mod;
  uint64_t nm = meta.p;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  //   NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  //   uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    switch_modulus(cache_A, om, nm, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");

    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int MOD() {
  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint32_t res = meta.res.start;
  uint64_t om = meta.mod;
  uint64_t nm = meta.p;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    modulus(cache_A, result, om, nm, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", result[i]);
    }
    printf("\n");

    mram_write(result, (__mram_ptr void *)(mram_base_addr_res + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int MODEq() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint64_t om = meta.mod;
  uint64_t nm = meta.p;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    modulus_Eq(cache_A, om, nm, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");

    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int MODByTwo() {
  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint32_t res = meta.res.start;
  uint64_t om = meta.mod;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    ModByTwo(cache_A, result, om, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", result[i]);
    }
    printf("\n");

    mram_write(result, (__mram_ptr void *)(mram_base_addr_res + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}
int MODByTwoEq() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint64_t om = meta.mod;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  //   NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  //   uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    ModByTwoEq(cache_A, om, size_to_copy);
    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");

    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int MODexp() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint32_t res = meta.res.start;
  uint64_t mv = meta.mod;
  uint64_t bv = meta.p;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    Modulus_Exp(cache_A, result, bv, mv, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", result[i]);
    }
    printf("\n");

    mram_write(result, (__mram_ptr void *)(mram_base_addr_res + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int MODexpEq() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint64_t mv = meta.mod;
  uint64_t bv = meta.p;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  //   NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  //   uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    Modular_Exp_Eq(cache_A, bv, mv, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");

    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int MODInverse() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint32_t res = meta.res.start;
  uint64_t mv = meta.mod;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    modular_inverse(cache_A, result, mv, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", result[i]);
    }
    printf("\n");

    mram_write(result, (__mram_ptr void *)(mram_base_addr_res + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int MODInverseEq() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint64_t mv = meta.mod;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  //   NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  //   uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    modular_inverse_eq(cache_A, mv, size_to_copy);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");

    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int MULTIPLYAndRound() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint32_t res = meta.res.start;
  uint64_t mv = meta.mod;
  uint64_t p = meta.p;
  uint64_t q = meta.q;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    MultiplyAndRound_vector(cache_A, result, mv, size_to_copy, p, q);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", result[i]);
    }
    printf("\n");

    mram_write(result, (__mram_ptr void *)(mram_base_addr_res + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}
int MULTIPLYAndRoundEq() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint64_t mv = meta.mod;
  uint64_t p = meta.p;
  uint64_t q = meta.q;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  //   NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  //   uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    MultiplyAndRoundEq_vector(cache_A, mv, size_to_copy, p, q);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");

    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}

int DIVIDEAndRound() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint32_t res = meta.res.start;
  uint64_t mv = meta.mod;
  uint64_t q = meta.p;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    DivideAndRound_vector(cache_A, result, size_to_copy, mv, q);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", result[i]);
    }
    printf("\n");

    mram_write(result, (__mram_ptr void *)(mram_base_addr_res + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}
int DIVIDEAndRoundEq() {

  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
    mem_reset(); // Resets the heap
  }
  barrier_wait(&my_barrier);
  uint32_t op = meta.op.start;
  uint32_t op_size = meta.op.size;
  uint64_t mv = meta.mod;
  uint64_t q = meta.p;

  uint32_t size_to_copy_bytes = CACHE_SIZE < op_size ? CACHE_SIZE : op_size;
  uint32_t size_to_copy = size_to_copy_bytes >> 3;

  NativeInt *cache_A = (NativeInt *)mem_alloc(size_to_copy_bytes);
  // NativeInt *cache_B = (NativeInt *)mem_alloc(size_to_copy_bytes);
  //   NativeInt *result = (NativeInt *)mem_alloc(size_to_copy_bytes);

  uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + op);
  // uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + op2);
  //   uint32_t mram_base_addr_res = (uint32_t)(DPU_MRAM_HEAP_POINTER + res);

  for (uint32_t bytes_index = tasklet_id * CACHE_SIZE; bytes_index < op_size;
       bytes_index += CACHE_SIZE * NR_TASKLETS) {

    mram_read((__mram_ptr void const *)(mram_base_addr_A + bytes_index),
              cache_A, size_to_copy_bytes);

    DivideAndRoundEq_vector(cache_A, size_to_copy, mv, q);

    for (int i = 0; i < size_to_copy; i++) {
      printf("%ld ", cache_A[i]);
    }
    printf("\n");

    mram_write(cache_A, (__mram_ptr void *)(mram_base_addr_A + bytes_index),
               size_to_copy_bytes);
  }
  return 0;
}
