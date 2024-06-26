#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

/*
    in this file i try to put the same computations as the one being executed on openFHE- just for fair comparison
    However, specific hardware optimizations can be made for some operations which are specific to DPUs, i.e 
    writting some code in assembly code of the DPU.
*/
typedef uint64_t NativeInt;
typedef int64_t SignedNativeInt;

struct typeD {
    NativeInt hi;
    NativeInt lo;
};

/* Functions associated with the polynomial modular element-wise additions */
// This is the number of elements  that is to be copied from the host
static void ModAddFastEq(NativeInt* m_value, NativeInt b_m_value, NativeInt modulus_m_values) {
    *m_value += b_m_value;

    if (*m_value >= modulus_m_values)
        *m_value -= modulus_m_values;
}

/* Functions associated with the polynomial modular element-wise multiplications */
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
    int64_t msb = GetMSB(m_value);

    /*I changed the original code to accomodate the shifts operations for divisions and multiplication*/
    NativeInt tmp = 1 << (msb << 1 | 3);

    if ((m_value & (m_value - 1)) == 0) {
        return tmp >> msb;  // Equivalent to tmp / m_value
    }
    else {
        return tmp / m_value;  // Use regular division
    }
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

/* Functions and operations associated with the NTT implementations */
static NativeInt MultDHi(NativeInt a, NativeInt b) {
    struct typeD x;
    MultD(a, b, &x);
    return x.hi;
}

void ModMulFastConstEq(NativeInt* m_value, NativeInt b_m_value, NativeInt modulus_m_value, NativeInt bInv_mvalue) {
    NativeInt q            = MultDHi(*m_value, bInv_mvalue) + 1;
    SignedNativeInt yprime = (SignedNativeInt)((*m_value) * b_m_value - q * modulus_m_value);
    (*m_value)             = (NativeInt)(yprime >= 0 ? yprime : yprime + modulus_m_value);
}
