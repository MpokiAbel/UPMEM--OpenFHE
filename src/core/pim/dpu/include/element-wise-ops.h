#include "operations.h"
#include <alloc.h>
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define CACHE_SIZE (1 << 8)
#define SIZE (CACHE_SIZE >> 3)

static void ModByTwo(NativeInt *m_data, NativeInt *res, NativeInt m_modulus,
                     size_t size) {
  NativeInt halfQ = m_modulus >> 1;
  for (size_t i = 0; i < size; ++i)
    res[i] = 0x1 & (m_data[i] ^ (m_data[i] > halfQ));
}

static void ModByTwoEq(NativeInt *m_data, NativeInt m_modulus, size_t size) {
  NativeInt halfQ = m_modulus >> 1;
  for (size_t i = 0; i < size; ++i)
    m_data[i] = 0x1 & (m_data[i] ^ (m_data[i] > halfQ));
}

static void Modulus_Exp(NativeInt *m_data, NativeInt *res, NativeInt bv,
                        NativeInt mv, size_t size) {
  if (bv >= mv)
    ModEq(&bv, mv);
  for (size_t i = 0; i < size; ++i)
    res[i] = ModExp(m_data[i], bv, mv);
}

static void Modular_Exp_Eq(NativeInt *m_data, NativeInt bv, NativeInt mv,
                           size_t size) {
  if (bv >= mv)
    ModEq(&bv, mv);
  for (size_t i = 0; i < size; ++i)
    m_data[i] = ModExp(m_data[i], bv, mv);
}

static void switch_modulus(NativeInt *m_data, NativeInt m_modulus,
                           NativeInt modulus, size_t size) {
  NativeInt halfQ = m_modulus >> 1;
  NativeInt om = m_modulus;
  NativeInt nm = modulus;
  if (nm > om) {
    NativeInt diff = nm - om;
    for (size_t i = 0; i < size; ++i) {
      NativeInt *v = &m_data[i];
      if (*v > halfQ)
        *v = *v + diff;
    }
  } else {
    NativeInt diff = nm - (om % nm);
    for (size_t i = 0; i < size; ++i) {
      NativeInt *v = &m_data[i];
      if (*v > halfQ)
        *v = *v + diff;
      if (*v >= nm)
        *v = *v % nm;
    }
  }
}

static void modulus(NativeInt *m_data, NativeInt *res, NativeInt m_modulus,
                    NativeInt modulus, size_t size) {
  if (modulus == 2)
    return ModByTwoEq(m_data, m_modulus, size);
  NativeInt nm = modulus;
  NativeInt halfQ = m_modulus >> 1;
  NativeInt om = m_modulus;
  if (nm > om) {
    NativeInt diff = nm - om;
    for (size_t i = 0; i < size; ++i) {
      NativeInt *v = &m_data[i];
      if (*v > halfQ)
        res[i] = *v + diff;
    }
  } else {
    NativeInt diff = nm - (om % nm);
    for (size_t i = 0; i < size; ++i) {
      NativeInt *v = &m_data[i];
      if (*v > halfQ)
        res[i] = *v + diff;
      if (*v >= nm)
        res[i] = *v % nm;
    }
  }
}

static void modulus_Eq(NativeInt *m_data, NativeInt m_modulus,
                       NativeInt modulus, size_t size) {
  if (modulus == 2)
    return ModByTwoEq(m_data, m_modulus, size);
  NativeInt nm = modulus;
  NativeInt halfQ = m_modulus >> 1;
  NativeInt om = m_modulus;
  if (nm > om) {
    NativeInt diff = nm - om;
    for (size_t i = 0; i < size; ++i) {
      NativeInt *v = &m_data[i];
      if (*v > halfQ)
        *v = *v + diff;
    }
  } else {
    NativeInt diff = nm - (om % nm);
    for (size_t i = 0; i < size; ++i) {
      NativeInt *v = &m_data[i];
      if (*v > halfQ)
        *v = *v + diff;
      if (*v >= nm)
        *v = *v % nm;
    }
  }
}

// addition operations
static void modular_addition_scalar(NativeInt *m_value, NativeInt b_m_value,
                                    NativeInt *res, NativeInt modulus,
                                    size_t size) {
  if (b_m_value >= modulus)
    ModEq(&b_m_value, modulus);
  for (size_t i = 0; i < size; i++)
    *(res + i) = ModAddFast(*(m_value + i), b_m_value, modulus);
}

static void modular_addition_scalar_Eq(NativeInt *m_value, NativeInt b_m_value,
                                       NativeInt modulus, size_t size) {
  // printf("%ld\n", b_m_value);
  if (b_m_value >= modulus)
    ModEq(&b_m_value, modulus);
  for (size_t i = 0; i < size; i++)
    ModAddFastEq(m_value + i, b_m_value, modulus);
}

static void modular_addition_vector(NativeInt *m_value, NativeInt *b_m_value,
                                    NativeInt *res, NativeInt modulus_m_values,
                                    size_t size) {
  for (size_t i = 0; i < size; i++)
    *(res + i) = ModAddFast(*(m_value + i), *(b_m_value + i), modulus_m_values);
}

static void modular_addition_vector_Eq(NativeInt *m_value, NativeInt *b_m_value,
                                       NativeInt modulus, size_t size) {
  for (size_t i = 0; i < size; i++)
    ModAddFastEq(m_value + i, *(b_m_value + i), modulus);
}

// subtraction operations
static void modular_subtraction_scalar(NativeInt *m_value, NativeInt b_m_value,
                                       NativeInt *res, NativeInt modulus,
                                       size_t size) {
  if (b_m_value >= modulus)
    ModEq(&b_m_value, modulus);
  for (size_t i = 0; i < size; i++)
    *(res + i) = ModSubFast(*(m_value + i), b_m_value, modulus);
}

static void modular_subtratcion_scalar_Eq(NativeInt *m_value,
                                          NativeInt b_m_value,
                                          NativeInt modulus, size_t size) {
  if (b_m_value >= modulus)
    ModEq(&b_m_value, modulus);
  for (size_t i = 0; i < size; i++)
    ModSubFastEq(m_value + i, b_m_value, modulus);
}

static void modular_subtraction_vector(NativeInt *m_value, NativeInt *b_m_value,
                                       NativeInt *res,
                                       NativeInt modulus_m_values,
                                       size_t size) {
  for (size_t i = 0; i < size; i++)
    *(res + i) = ModSubFast(*(m_value + i), *(b_m_value + i), modulus_m_values);
}

static void modular_subtraction_vector_Eq(NativeInt *m_value,
                                          NativeInt *b_m_value,
                                          NativeInt modulus, size_t size) {
  for (size_t i = 0; i < size; i++)
    ModSubFastEq(m_value + i, *(b_m_value + i), modulus);
}

// Multiplication operations
static void modular_multiplication_scalar(NativeInt *m_value,
                                          NativeInt b_m_value, NativeInt *res,
                                          NativeInt modulus, size_t size) {
  if (b_m_value >= modulus)
    ModEq(&b_m_value, modulus);
  NativeInt mu = ComputeMu(modulus);
  for (size_t i = 0; i < size; i++)
    *(res + i) = ModMulFastB(*(m_value + i), b_m_value, modulus, mu);
}

static void modular_multiplication_scalar_Eq(NativeInt *m_value,
                                             NativeInt b_m_value,
                                             NativeInt modulus, size_t size) {
  if (b_m_value >= modulus)
    ModEq(&b_m_value, modulus);
  NativeInt mu = ComputeMu(modulus);
  for (size_t i = 0; i < size; i++)
    ModMulFastEqB(m_value + i, b_m_value, modulus, mu);
}

static void modular_multiplication_vector(NativeInt *m_value,
                                          NativeInt *b_m_value, NativeInt *res,
                                          NativeInt modulus, size_t size) {
  NativeInt mu = ComputeMu(modulus);
  for (size_t i = 0; i < size; i++)
    *(res + i) = ModMulFastB(*(m_value + i), *(b_m_value + i), modulus, mu);
}

static void modular_multiplication_vector_Eq(NativeInt *m_value,
                                             NativeInt *b_m_value,
                                             NativeInt modulus, size_t size) {
  NativeInt mu = ComputeMu(modulus);
  for (size_t i = 0; i < size; i++)
    ModMulFastEqB(m_value + i, *(b_m_value + i), modulus, mu);
}

void MultWithOutMod(NativeInt *m_data, NativeInt *b, NativeInt *res,
                    size_t size) {
  for (size_t i = 0; i < size; ++i)
    res[i] = m_data[i] * b[i];
}

void MultiplyAndRound_vector(NativeInt *m_data, NativeInt *res,
                             NativeInt m_modulus, size_t size, NativeInt p,
                             NativeInt q) {
  NativeInt halfQ = m_modulus >> 1;
  for (size_t i = 0; i < size; ++i) {
    if (m_data[i] > halfQ) {
      NativeInt tmp = m_modulus - m_data[i];
      res[i] = m_modulus - MultiplyAndRound(tmp, p, q);
    } else {
      res[i] = Mod(MultiplyAndRound(m_data[i], p, q), m_modulus);
    }
  }
}

void MultiplyAndRoundEq_vector(NativeInt *m_data, NativeInt m_modulus,
                               size_t size, NativeInt p, NativeInt q) {
  MultiplyAndRound_vector(m_data, m_data, m_modulus, size, p, q);
}

void DivideAndRound_vector(NativeInt *m_data, NativeInt *res, size_t size,
                           NativeInt m_modulus, NativeInt q) {
  NativeInt halfQ = m_modulus >> 1;
  NativeInt mv = m_modulus;
  for (size_t i = 0; i < size; ++i) {
    if (m_data[i] > halfQ) {
      NativeInt tmp = mv - m_data[i];
      res[i] = mv - DivideAndRound(tmp, q);
    } else {
      res[i] = DivideAndRound(m_data[i], q);
    }
  }
}

void DivideAndRoundEq_vector(NativeInt *m_data, size_t size,
                             NativeInt m_modulus, NativeInt q) {
  DivideAndRound_vector(m_data, m_data, size, m_modulus, q);
}

void modular_inverse(NativeInt *m_data, NativeInt *res, NativeInt m_modulus,
                     size_t size) {
  for (size_t i = 0; i < size; ++i)
    res[i] = ModInverse(m_data[i], m_modulus);
}

void modular_inverse_eq(NativeInt *m_data, NativeInt m_modulus, size_t size) {
  modular_inverse(m_data, m_data, m_modulus, size);
}