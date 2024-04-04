#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

/*
    This file tries to put the same computations as the one being executed on openFHE- just for fair comparison
    However, specific hardware optimizations can be made for some operations which are specific to DPUs, i.e 
    writting some code in assemby code of the DPU.
*/


typedef uint64_t NativeInt;

struct typeD {
    NativeInt hi;
    NativeInt lo;
};

// This is the number of elements  that is to be copied from the host
static void ModAddFastEq(NativeInt* m_value, NativeInt b_m_value, NativeInt modulus_m_values) {
    *m_value += b_m_value;

    if (*m_value >= modulus_m_values)
        *m_value -= modulus_m_values;
}

// Performing the ModAddEq operation make sure to cast the operations to 64 bits operations
static void ModAddEq(NativeInt* m_value, NativeInt* b_m_value, NativeInt modulus_m_values, size_t size) {
    for (size_t i = 0; i < size; i++) {
        ModAddFastEq(m_value + i, *(b_m_value + i), modulus_m_values);
    }
}

static NativeInt RShiftD(const struct typeD* x, int64_t shift) {
    const int MaxBits = 64;  // Assuming 64 bits for NativeInt
    return (x->lo >> shift) | (x->hi << (MaxBits - shift));
}

/*ToDo: check for assembly implementation of this algorithm it could be much faster*/
static void MultD(NativeInt a, NativeInt b, struct typeD* result) {
    NativeInt aHigh = a >> 32;
    NativeInt aLow  = a & 0xFFFFFFFF;
    NativeInt bHigh = b >> 32;
    NativeInt bLow  = b & 0xFFFFFFFF;

    NativeInt p1 = aHigh * bHigh;
    NativeInt p2 = aLow * bLow;
    NativeInt p3 = (aHigh + aLow) * (bHigh + bLow) - p1 - p2;

    // Combine the partial results to get the final high and low parts.
    result->lo = p2 + ((p3 & 0xFFFFFFFF) << 32);
    result->hi = p1 + (p3 >> 32) + ((p2 + ((p3 & 0xFFFFFFFF) << 32)) < p2);
}

static void SubtractD(struct typeD* res, struct typeD a) {
    if (res->lo < a.lo) {
        res->lo += UINT64_MAX + 1 - a.lo;
        res->hi--;
    }
    else {
        res->lo -= a.lo;
    }
    res->hi -= a.hi;
}

static unsigned int GetMSB(NativeInt n) {
    if (n == 0)
        return 0;
#if defined(_MSC_VER)
    unsigned long msb;
    _BitScanReverse64(&msb, x);
    return (unsigned int)msb + 1;
#elif defined(__GNUC__) || defined(__clang__)
    return 64 - __builtin_clzll(n);
#else
    #error "Compiler not supported for GetMSB64"
#endif
}

NativeInt ComputeMu(NativeInt m_value) {
    if (m_value == 0) {
        return 0;
    }

    NativeInt tmp = 1 << (2 * GetMSB(m_value) + 3);
    return tmp / m_value;
}

/* Here the DNative is the same as native However we would need to simulate the 128 bit operation*/
static void ModMulFastEq(NativeInt* m_value, NativeInt b_value, NativeInt modulus_value, NativeInt mu_value) {
    struct typeD tmp;
    int64_t n = GetMSB(modulus_value) - 2;
    MultD(*m_value, b_value, &tmp);
    struct typeD r = tmp;
    MultD(RShiftD(&tmp, n), mu_value, &tmp);
    MultD(modulus_value, RShiftD(&tmp, n + 7), &tmp);
    SubtractD(&r, tmp);
    *m_value = r.lo;
    if (*m_value >= modulus_value)
        *m_value -= modulus_value;
}

static void ModMulNoCheckEq(NativeInt* m_value, NativeInt* b_m_value, NativeInt modulus_m_values, NativeInt mu_value,
                            size_t size) {
    // auto mu{m_modulus.ComputeMu()};
    for (size_t i = 0; i < size; ++i)
        ModMulFastEq(m_value + i, *(b_m_value + i), modulus_m_values, mu_value);
}

/*The next function to be implemented is the ntt and intt operation*/
