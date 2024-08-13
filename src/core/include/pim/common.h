#ifndef __PIM_COMMON__
#define __PIM_COMMON__

#include <stdint.h>

#define DIVROUNDUP(x, y) (((x) + (y)-1) / (y))
#define DIV(x, y) ((x) / (y))

struct operands {
  uint64_t start;
  uint32_t size;
};

enum add_sub_mul_kernel {
  SCALAR = 0,
  SCALAR_EQ = 1,
  VECTOR = 2,
  VECTOR_EQ = 3,
};

// Arguments for the modular operations kernels with normal and barret variants
struct mod_add_sub_mult {
  struct operands op1;
  struct operands op2;
  struct operands res;
  uint64_t mod;
  uint64_t mu;
  enum add_sub_mul_kernel kernel;
};

enum mod_kernel {
  SWITCHMODULUS = 0,
  MOD_NORMAL = 1,
  MODEQ = 2,
  MODBYTWO = 3,
  MODBYTWOEQ = 4,
  MODEXP = 5,
  MODEXPEQ = 6,
  MODINVERSE = 7,
  MODINVERSEEQ = 8,
  MULTIPLYANDROUND = 9,
  MULTIPLYANDROUNDEQ = 10,
  DIVIDEANDROUND = 11,
  DIVIDEANDROUNDEQ = 12
};

struct mod_ops {
  struct operands op;
  struct operands res;
  uint64_t mod;
  uint64_t p;
  uint64_t q;
  enum mod_kernel kernel;
};

// Arguments for the

#endif //__PIM_COMMON__