#ifndef DPU_H
#define DPU_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t modulus;  // The modulus of the poly
    uint64_t size;     // The dimension of the polynomial
    uint64_t* vec;     // An array values of the polynomial
} PolyDpu;

typedef struct {
    size_t size;    // Number of poly available
    PolyDpu* Poly;  // Pointer to an array of PolyParams, of length 'size'
} DCRTPolyDpu;

#endif  // DPU_H
