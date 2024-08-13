#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
  In this file i try to put the same computations as the one being executed on
   openFHE- just for fair comparison However, specific hardware optimizations
   can be made for some operations which are specific to DPUs, i.e writting some
   code in assembly code of the DPU.
*/
typedef uint64_t NativeInt;
typedef int64_t SignedNativeInt;

struct typeD {
  NativeInt hi;
  NativeInt lo;
};

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

static NativeInt RShiftD(const struct typeD *x, int64_t shift) {
  const int MaxBits = 64; // Assuming 64 bits for NativeInt
  return (x->lo >> shift) | (x->hi << (MaxBits - shift));
}

static void MultD(NativeInt a, NativeInt b, struct typeD *result) {
  NativeInt aHigh = a >> 32;
  NativeInt aLow = a & 0xFFFFFFFF;
  NativeInt bHigh = b >> 32;
  NativeInt bLow = b & 0xFFFFFFFF;

  NativeInt p1 = aHigh * bHigh;
  NativeInt p2 = aLow * bLow;
  NativeInt p3 = (aHigh + aLow) * (bHigh + bLow) - p1 - p2;

  // Combine the partial results to get the final high and low parts.
  result->lo = p2 + ((p3 & 0xFFFFFFFF) << 32);
  result->hi = p1 + (p3 >> 32) + ((p2 + ((p3 & 0xFFFFFFFF) << 32)) < p2);
}

static void SubtractD(struct typeD *res, struct typeD a) {
  if (res->lo < a.lo) {
    res->lo += UINT64_MAX + 1 - a.lo;
    res->hi--;
  } else {
    res->lo -= a.lo;
  }
  res->hi -= a.hi;
}

NativeInt ComputeMu(NativeInt m_value) {
  if (m_value == 0) {
    return 0;
  }
  int64_t msb = GetMSB(m_value);

  /*I changed the original code to accomodate the shifts operations for
   * divisions and multiplication*/
  NativeInt tmp = 1 << (msb << 1 | 3);

  if ((m_value & (m_value - 1)) == 0) {
    return tmp >> msb; // Equivalent to tmp / m_value
  } else {
    return tmp / m_value; // Use regular division
  }
}

// functions associated with modulus operations
static NativeInt Mod(NativeInt m_value, const NativeInt modulus) {
  return m_value % modulus;
}

static void ModEq(NativeInt *m_value, const NativeInt modulus) {
  *m_value = *m_value % modulus;
}

static void ModMu(struct typeD prod, NativeInt *a, const NativeInt mv,
                  const NativeInt mu, int64_t n) {
  prod.hi = 0;
  prod.lo = *a;
  MultD(RShiftD(&prod, n), mu, &prod);
  MultD(RShiftD(&prod, n + 7), mv, &prod);
  *a -= prod.lo;
  if (*a >= mv)
    *a -= mv;
}

// barret ops
static NativeInt ModB(NativeInt m_value, const NativeInt modulus,
                      const NativeInt mu) {
  struct typeD tmp;
  ModMu(tmp, &m_value, modulus, mu, GetMSB(modulus) - 2);
  return m_value;
}

static void ModEqB(NativeInt *m_value, const NativeInt modulus,
                   const NativeInt mu) {
  struct typeD tmp;
  ModMu(tmp, m_value, modulus, mu, GetMSB(modulus) - 2);
}

// functions associated with modulus additions
NativeInt ModAdd(NativeInt av, NativeInt bv, const NativeInt mv) {
  if (av >= mv)
    av %= mv;
  if (bv >= mv)
    bv %= mv;
  av += bv;
  if (av >= mv)
    av -= mv;
  return av;
}

// In-place variant
void ModAddEq(NativeInt *a, const NativeInt *b, const NativeInt mv) {
  NativeInt bv = *b;

  if (*a >= mv)
    *a = *a % mv;

  if (bv >= mv)
    bv = bv % mv;

  *a += bv;

  if (*a >= mv)
    *a -= mv;
}

// When the operands are less than modulus go faster
// Normal:
NativeInt ModAddFast(NativeInt av, NativeInt bv, const NativeInt mv) {
  NativeInt r = av + bv;
  if (r >= mv)
    r -= mv;
  return r;
}

// in-place variants
void ModAddFastEq(NativeInt *a, const NativeInt bv, const NativeInt mv) {
  *a += bv;
  if (*a >= mv)
    *a -= mv;

  // printf("%ld,%ld ",*a,bv);
}

// Barret's addition operations

// Normal:
NativeInt ModAddB(NativeInt av, NativeInt bv, const NativeInt mv,
                  const NativeInt mu) {
  if (av >= mv)
    ModEqB(&av, mv, mu);
  if (bv >= mv)
    ModEqB(&bv, mv, mu);
  av += bv;
  if (av >= mv)
    av -= mv;
  return av;
}

// In-place variant

void ModAddEqB(NativeInt *av, NativeInt *bv, const NativeInt mv,
               const NativeInt mu) {
  if (*av >= mv)
    ModEqB(av, mv, mu);
  if (*bv >= mv)
    ModEqB(bv, mv, mu);
  *av = *av + *bv;
  if (*av >= mv)
    *av -= mv;
}

// Modulus substractions operations
// Normal
NativeInt ModSub(NativeInt av, NativeInt bv, const NativeInt mv) {
  if (av >= mv)
    av %= mv;
  if (bv >= mv)
    bv %= mv;
  if (av < bv)
    return av + mv - bv;
  return av - bv;
}

// In-place variants
void ModSubEq(NativeInt *av, NativeInt *bv, const NativeInt mv) {
  if (*av >= mv)
    *av = *av % mv;
  if (*bv >= mv)
    *bv = *bv % mv;
  if (*av < *bv)
    *av = *av + mv - *bv;
  *av = *av - *bv;
}

// Faster modular multiplications for operands < Modulus
// Normal:
NativeInt ModSubFast(NativeInt av, const NativeInt bv, const NativeInt mv) {
  if (mv < bv)
    return av + mv - bv;
  return av - bv;
}

// in-place variants:
void ModSubFastEq(NativeInt *av, NativeInt bv, const NativeInt mv) {
  if (*av < bv)
    *av = *av + mv - bv;
  *av = *av - bv;
}

// barret subtractions

NativeInt ModSubB(NativeInt av, NativeInt bv, const NativeInt mv,
                  const NativeInt mu) {
  if (av >= mv)
    ModEqB(&av, mv, mu);
  if (bv >= mv)
    ModEqB(&bv, mv, mu);
  if (av < bv)
    return av + mv - bv;
  return av - bv;
}

void ModSubEqB(NativeInt *av, NativeInt *bv, const NativeInt mv,
               const NativeInt mu) {
  if (*av >= mv)
    ModEqB(av, mv, mu);
  if (*bv >= mv)
    ModEqB(bv, mv, mu);
  if (av < bv)
    *av = *av + mv - *bv;
  *av = *av - *bv;
}

/* Functions associated with the polynomial modular element-wise multiplications
 */

NativeInt ModMul(NativeInt av, NativeInt bv, const NativeInt mv) {
  struct typeD tmp;
  NativeInt mu = ComputeMu(mv);
  int64_t n = GetMSB(mv) - 2;
  if (av >= mv)
    ModMu(tmp, &av, mv, mu, n);
  if (bv >= mv)
    ModMu(tmp, &bv, mv, mu, n);
  MultD(av, bv, &tmp);
  struct typeD r = tmp;
  MultD(RShiftD(&tmp, n), mu, &tmp);
  MultD(RShiftD(&tmp, n + 7), mv, &tmp);
  SubtractD(&r, tmp);
  if (r.lo >= mv)
    r.lo -= mv;
  return r.lo;
}

void ModMulEq(NativeInt *av, NativeInt bv, const NativeInt mv) {
  struct typeD tmp;
  NativeInt mu = ComputeMu(mv);
  int64_t n = GetMSB(mv) - 2;
  if (*av >= mv)
    ModMu(tmp, av, mv, mu, n);
  if (bv >= mv)
    ModMu(tmp, &bv, mv, mu, n);
  MultD(*av, bv, &tmp);
  struct typeD r = tmp;
  MultD(RShiftD(&tmp, n), mu, &tmp);
  MultD(RShiftD(&tmp, n + 7), mv, &tmp);
  SubtractD(&r, tmp);
  *av = r.lo;
  if (r.lo >= mv)
    av -= mv;
}

// Barret reduction
NativeInt ModMulB(NativeInt av, NativeInt bv, const NativeInt mv,
                  const NativeInt mu) {
  struct typeD tmp;
  int64_t n = GetMSB(mv) - 2;
  if (av >= mv)
    ModMu(tmp, &av, mv, mu, n);
  if (bv >= mv)
    ModMu(tmp, &bv, mv, mu, n);
  MultD(av, bv, &tmp);
  struct typeD r = tmp;
  MultD(RShiftD(&tmp, n), mu, &tmp);
  MultD(RShiftD(&tmp, n + 7), mv, &tmp);
  SubtractD(&r, tmp);
  if (r.lo >= mv)
    r.lo -= mv;
  return r.lo;
}

void ModMulEqB(NativeInt *av, NativeInt bv, const NativeInt mv,
               const NativeInt mu) {
  int64_t n = GetMSB(mv) - 2;
  struct typeD tmp;
  if (*av >= mv)
    ModMu(tmp, av, mv, mu, n);
  if (bv >= mv)
    ModMu(tmp, &bv, mv, mu, n);
  MultD(*av, bv, &tmp);
  struct typeD r = tmp;
  MultD(RShiftD(&tmp, n), mu, &tmp);
  MultD(RShiftD(&tmp, n + 7), mv, &tmp);
  SubtractD(&r, tmp);
  *av = r.lo;
  if (r.lo >= mv)
    *av -= mv;
}

// operands are < modulus hence go faster
NativeInt ModMulFast(NativeInt av, NativeInt bv, const NativeInt mv) {
  int64_t n = GetMSB(mv) - 2;
  struct typeD prod;
  MultD(av, bv, &prod);
  struct typeD r = prod;
  MultD(RShiftD(&prod, n), ComputeMu(mv), &prod);
  MultD(RShiftD(&prod, n + 7), mv, &prod);
  SubtractD(&r, prod);
  if (r.lo >= mv)
    r.lo -= mv;
  return r.lo;
}

void ModMulFastEq(NativeInt *av, NativeInt bv, NativeInt mv) {
  int64_t n = GetMSB(mv) - 2;
  struct typeD prod;
  MultD(*av, bv, &prod);
  struct typeD r = prod;
  MultD(RShiftD(&prod, n), ComputeMu(mv), &prod);
  MultD(RShiftD(&prod, n + 7), mv, &prod);
  SubtractD(&r, prod);
  *av = r.lo;
  if (r.lo >= mv)
    *av -= mv;
}

// Barret variant
NativeInt ModMulFastB(NativeInt av, NativeInt bv, const NativeInt mv,
                      const NativeInt mu) {
  int64_t n = GetMSB(mv) - 2;
  struct typeD prod;
  MultD(av, bv, &prod);
  struct typeD r = prod;
  MultD(RShiftD(&prod, n), mu, &prod);
  MultD(RShiftD(&prod, n + 7), mv, &prod);
  SubtractD(&r, prod);
  if (r.lo >= mv)
    r.lo -= mv;
  return r.lo;
}

void ModMulFastEqB(NativeInt *av, NativeInt bv, const NativeInt mv,
                   const NativeInt mu) {
  int64_t n = GetMSB(mv) - 2;
  struct typeD prod;
  MultD(*av, bv, &prod);
  struct typeD r = prod;
  MultD(RShiftD(&prod, n), mu, &prod);
  MultD(RShiftD(&prod, n + 7), mv, &prod);
  SubtractD(&r, prod);
  *av = r.lo;
  if (r.lo >= mv)
    *av -= mv;
}

/* Functions and operations associated with the NTT implementations */
static NativeInt MultDHi(NativeInt a, NativeInt b) {
  struct typeD x;
  MultD(a, b, &x);
  return x.hi;
}

void ModMulFastConstEq(NativeInt *m_value, NativeInt b_m_value,
                       NativeInt modulus_m_value, NativeInt bInv_mvalue) {
  NativeInt q = MultDHi(*m_value, bInv_mvalue) + 1;
  SignedNativeInt yprime =
      (SignedNativeInt)((*m_value) * b_m_value - q * modulus_m_value);
  (*m_value) = (NativeInt)(yprime >= 0 ? yprime : yprime + modulus_m_value);
}

NativeInt ModExp(NativeInt m_value, NativeInt b, NativeInt mod) {
  NativeInt t = m_value % mod;
  NativeInt mu = ComputeMu(mod);
  NativeInt r = 1;
  if (b & 0x1)
    ModMulFastEqB(&r, t, mod, mu);
  while (b >>= 1) {
    ModMulFastEqB(&t, t, mod, mu);
    if (b & 0x1)
      ModMulFastEqB(&r, t, mod, mu);
  }
  return r;
}

NativeInt MultiplyAndRound(NativeInt data, NativeInt p, NativeInt q) {

  if (q == 0) {
    printf("Division by Zero");
  }
  NativeInt halfQ = q >> 1;
  NativeInt ans = data * q;
  NativeInt rounded_res = (ans + halfQ) / q;

  return rounded_res;
}

void MultiplyAndRoundEq(NativeInt *data, NativeInt p, NativeInt q) {
  *data = MultiplyAndRound(*data, p, q);
}

NativeInt DivideAndRound(NativeInt m_value, NativeInt q) {
  if (q == 0)
    printf(" DivideAndRound: zero");
  NativeInt ans = m_value / q;
  NativeInt rem = m_value % q;
  NativeInt halfQ = q >> 1;
  if (rem > halfQ)
    return ans + 1;
  return ans;
}

void DivideAndRoundEq(NativeInt *m_value, NativeInt q) {
  *m_value = DivideAndRound(*m_value, q);
}

NativeInt ModInverse(NativeInt m_value, NativeInt mod) {
  SignedNativeInt modulus = mod;
  SignedNativeInt a = m_value % mod;
  if (a == 0) {
    printf("Does no have a mod inverse\n");
  }
  if (modulus == 1)
    return 0;

  SignedNativeInt y = 0;
  SignedNativeInt x = 1;
  while (a > 1) {
    SignedNativeInt t = modulus;
    SignedNativeInt q = a / t;
    modulus = a % t;
    a = t;
    t = y;
    y = x - q * y;
    x = t;
  }
  if (x < 0)
    x += mod;
  return x;
}