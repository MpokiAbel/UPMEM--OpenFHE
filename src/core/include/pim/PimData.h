#ifndef _PIM_DATA_
#define _PIM_DATA_

#include "PimManager.h"
#include "kernel.h"
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdint.h>
#include <sys/types.h>

class PimData {
public:
  PimData(std::initializer_list<uint64_t> data) : host_data(data) {
    pim = PimManager::getPim(4);
    size = data.size();
    metadata = pim->allocate(size);
    pim->copy_to_pim(host_data.data(), size, metadata[0].second);
  };

  PimData(uint64_t size_set) : size(size_set) {
    pim = PimManager::getPim(1);
    metadata = pim->allocate(size);
  };

  PimData(){};

  ~PimData() { pim->deallocate(metadata); }

  void sync() {
    pim->copy_from_pim(host_data.data(), size, metadata[0].second);
  }

  PimData &operator=(const PimData &rhs) {
    if (this != &rhs) {
      size = rhs.size;
      pim = rhs.pim;
      metadata.resize(rhs.metadata.size());
      std::copy(rhs.metadata.begin(), rhs.metadata.end(), metadata.begin());
    }
    return *this;
  }

  PimData &operator+=(const uint64_t &rhs) {
    do_ops(rhs, SCALAR_EQ, ELEM_MOD_ADD);
    return *this;
  }

  PimData &operator+=(const PimData &rhs) {
    do_ops(rhs, VECTOR_EQ, ELEM_MOD_ADD);
    return *this;
  }

  PimData operator+(const PimData &rhs) {
    PimData ret(size);
    do_ops(rhs, ret, VECTOR, ELEM_MOD_ADD);
    return ret;
  }

  PimData operator+(const uint64_t &rhs) {
    PimData ret(size);
    do_ops(rhs, ret, SCALAR, ELEM_MOD_ADD);
    return ret;
  }

  PimData &operator-=(const uint64_t &rhs) {
    do_ops(rhs, SCALAR_EQ, ELEM_MOD_SUB);
    return *this;
  }

  PimData &operator-=(const PimData &rhs) {
    do_ops(rhs, VECTOR_EQ, ELEM_MOD_SUB);
    return *this;
  }
  PimData operator-(const PimData &rhs) {
    PimData result(size);
    do_ops(rhs, result, VECTOR, ELEM_MOD_SUB);
    return result;
  }

  PimData operator-(const uint64_t &rhs) {
    PimData result(size);
    do_ops(rhs, result, SCALAR, ELEM_MOD_SUB);
    return result;
  }

  PimData &operator*=(const uint64_t &rhs) {
    do_ops(rhs, SCALAR_EQ, ELEM_MOD_MULT);
    return *this;
  }

  PimData &operator*=(const PimData &rhs) {
    do_ops(rhs, VECTOR_EQ, ELEM_MOD_MULT);
    return *this;
  }

  PimData operator*(const PimData &rhs) {
    PimData result(size);
    do_ops(rhs, result, VECTOR, ELEM_MOD_MULT);
    return result;
  }

  PimData operator*(const uint64_t &rhs) {
    PimData result(size);
    do_ops(rhs, result, SCALAR, ELEM_MOD_MULT);
    return result;
  }

  // Addition operations
  PimData ModAdd(const uint64_t &rhs, const uint64_t &modulus) {
    PimData ret(size);
    do_ops(rhs, ret, SCALAR, ELEM_MOD_ADD, modulus);
    return ret;
  }

  PimData ModAdd(const PimData &rhs, const uint64_t &modulus) {
    PimData ret(size);
    do_ops(rhs, ret, VECTOR, ELEM_MOD_ADD, modulus);
    return ret;
  }

  void ModAddEq(const uint64_t &rhs, const uint64_t &modulus) {
    do_ops(rhs, SCALAR_EQ, ELEM_MOD_ADD, modulus);
  }
  void ModAddEq(const PimData &rhs, const uint64_t &modulus) {
    do_ops(rhs, VECTOR_EQ, ELEM_MOD_ADD, modulus);
  }

  // Substraction operations
  PimData ModSub(const uint64_t &rhs, const uint64_t &modulus) {
    PimData ret(size);
    do_ops(rhs, ret, SCALAR, ELEM_MOD_SUB, modulus);
    return ret;
  }
  PimData ModSub(const PimData &rhs, const uint64_t &modulus) {
    PimData ret(size);
    do_ops(rhs, ret, VECTOR, ELEM_MOD_SUB, modulus);
    return ret;
  }

  void ModSubEq(const uint64_t &rhs, const uint64_t &modulus) {
    do_ops(rhs, SCALAR_EQ, ELEM_MOD_SUB, modulus);
  }

  void ModSubEq(const PimData &rhs, const uint64_t &modulus) {
    do_ops(rhs, VECTOR_EQ, ELEM_MOD_SUB, modulus);
  }

  // Multiplication operations
  PimData ModMul(const uint64_t &rhs, const uint64_t &modulus) {
    PimData ret(size);
    do_ops(rhs, ret, SCALAR, ELEM_MOD_MULT, modulus);
    return ret;
  }
  PimData ModMul(const PimData &rhs, const uint64_t &modulus) {
    PimData ret(size);
    do_ops(rhs, ret, VECTOR, ELEM_MOD_MULT, modulus);
    return ret;
  }

  void ModMulEq(const uint64_t &rhs, const uint64_t &modulus) {
    do_ops(rhs, SCALAR_EQ, ELEM_MOD_MULT, modulus);
  }
  void ModMulEq(const PimData &rhs, const uint64_t &modulus) {
    do_ops(rhs, VECTOR_EQ, ELEM_MOD_MULT, modulus);
  }
  void MultWithOutMod(const PimData &rhs) {}

  // ToDo: connect the kernels from the mod-ops with the interfaces below.

  // mod operations
  PimData Mod(const uint64_t &om, const uint64_t &nm) {
    PimData ret(size);
    do_mod_ops(MOD_NORMAL, ret.metadata[0].second, ret.size, om, nm, 0);
    return ret;
  }
  void ModEq(const uint64_t &om, const uint64_t &nm) {
    do_mod_ops(MODEQ, 0, 0, om, nm, 0);
  }
  void switchmod(const uint64_t &om, const uint64_t &nm) {
    do_mod_ops(SWITCHMODULUS, 0, 0, om, nm, 0);
  }

  PimData ModByTwo(const uint64_t &modulus) {
    PimData ret(size);
    do_mod_ops(MODBYTWO, ret.metadata[0].second, ret.size, modulus, 0, 0);
    return ret;
  }
  void ModByTwoEq(const uint64_t &modulus) {
    do_mod_ops(MODBYTWOEQ, 0, 0, modulus, 0, 0);
  }

  PimData ModExp(const uint64_t &modulus, const uint64_t b) {
    PimData ret(size);
    do_mod_ops(MODEXP, ret.metadata[0].second, ret.size, modulus, b, 0);
    return ret;
  }
  void ModExpEq(const uint64_t &modulus, const uint64_t b) {
    do_mod_ops(MODEXPEQ, 0, 0, modulus, b, 0);
  }

  PimData ModInverse(uint64_t modulus) {
    PimData ret(size);
    do_mod_ops(MODINVERSE, ret.metadata[0].second, ret.size, modulus, 0, 0);
    return ret;
  }
  void ModInverseEq(uint64_t modulus) {
    do_mod_ops(MODINVERSEEQ, 0, 0, modulus, 0, 0);
  }

  PimData MultiplyAndRound(const uint64_t &modulus, const uint64_t &p,
                           const uint64_t &q) {
    PimData ret(size);
    do_mod_ops(MULTIPLYANDROUND, ret.metadata[0].second, ret.size, modulus, p,
               q);
    return ret;
  }

  void MultiplyAndRoundEq(const uint64_t &modulus, const uint64_t &p,
                          const uint64_t &q) {
    do_mod_ops(MULTIPLYANDROUNDEQ, 0, 0, modulus, p, q);
  }

  PimData DivideAndRound(const uint64_t &modulus, const uint64_t &q) {
    PimData ret(size);
    do_mod_ops(DIVIDEANDROUND, ret.metadata[0].second, ret.size, modulus, q, 0);
    return ret;
  }
  void DivideAndRoundEq(const uint64_t &modulus, const uint64_t &q) {
    do_mod_ops(DIVIDEANDROUNDEQ, 0, 0, modulus, q, 0);
  }

private:
  // Normal operations

  void do_ops(const PimData &rhs, const PimData &res,
              enum add_sub_mul_kernel kernel, const std::string &ops,
              uint64_t mod = std::numeric_limits<uint64_t>::max(),
              uint64_t mu = 0) {
    common_do_ops(metadata[0].second, size * sizeof(uint64_t),
                  rhs.metadata[0].second, rhs.size * sizeof(uint64_t),
                  res.metadata[0].second, res.size, kernel, ops, mod, mu,
                  "Dot operation of vectors");
  }

  void do_ops(const uint64_t &rhs, const PimData &res,
              enum add_sub_mul_kernel kernel, const std::string &ops,
              uint64_t mod = std::numeric_limits<uint64_t>::max(),
              uint64_t mu = 0) {
    common_do_ops(metadata[0].second, size * sizeof(uint64_t), rhs, 0,
                  res.metadata[0].second, res.size, kernel, ops, mod, mu,
                  "Scalar operation of vectors");
  }

  // In place variants operations
  void do_ops(const PimData &rhs, enum add_sub_mul_kernel kernel,
              const std::string &ops,
              uint64_t mod = std::numeric_limits<uint64_t>::max(),
              uint64_t mu = 0) {
    common_do_ops(metadata[0].second, size * sizeof(uint64_t),
                  rhs.metadata[0].second, rhs.size * sizeof(uint64_t), 0, 0,
                  kernel, ops, mod, mu, "Dot operation of vectors");
  }

  void do_ops(const uint64_t &rhs, enum add_sub_mul_kernel kernel,
              const std::string &ops,
              uint64_t mod = std::numeric_limits<uint64_t>::max(),
              uint64_t mu = 0) {
    common_do_ops(metadata[0].second, size * sizeof(uint64_t), rhs, 0, 0, 0,
                  kernel, ops, mod, mu, "Scalar operation of vectors");
  }

  void common_do_ops(uint64_t op1_start, uint64_t op1_size, uint64_t op2_start,
                     uint64_t op2_size, uint64_t res_start, uint64_t res_size,
                     enum add_sub_mul_kernel kernel, const std::string &ops,
                     uint64_t mod, uint64_t mu,
                     const std::string &operation_desc) {
    // Create the metadata header
    struct mod_add_sub_mult meta;
    meta.op1.start = op1_start;
    meta.op1.size = DIV(op1_size, pim->getNumDpus());
    meta.op2.start = op2_start;
    meta.op2.size = DIV(op2_size, pim->getNumDpus());
    meta.mod = mod;
    meta.mu = mu;
    meta.res.start = res_start;
    meta.res.size = DIV(res_size, pim->getNumDpus());
    meta.kernel = kernel;
    std::cout << operation_desc << " " << meta.op1.start << " and "
              << meta.op2.start << std::endl;

    // Load the appropriate kernel
    pim->load_kernel(ops.c_str());

    // Send by broadcast the meta header to the DPUs, here not to the MRAM but
    // to the WRAM where the header resides
    pim->copy_to_pim(&meta, sizeof(meta), 0, 1, "meta");

    // Launch the kernel
    pim->start_kernel();

    pim->display();
  }

  void do_mod_ops(enum mod_kernel kernel, u_int32_t res_start,
                  u_int32_t res_size, uint64_t mod, uint64_t p, uint64_t q) {
    struct mod_ops ops;
    ops.kernel = kernel;
    ops.op.start = metadata[0].second;
    ops.op.size = DIV(size * sizeof(uint64_t), pim->getNumDpus());
    ops.res.start = res_start;
    ops.res.size = DIV(res_size * sizeof(uint64_t), pim->getNumDpus());
    ops.mod = mod;
    ops.p = p;
    ops.q = q;

    pim->load_kernel(ELEM_MOD_OPS);

    // Send by broadcast the meta header to the DPUs, here not to the MRAM but
    // to the WRAM where the header resides.
    pim->copy_to_pim(&ops, sizeof(ops), 0, 1, "meta");

    // Launch the kernel
    pim->start_kernel();

    pim->display();
  }

  std::vector<uint64_t> host_data;
  uint32_t size;
  PimManager *pim;
  std::vector<std::pair<size_t, uint32_t>> metadata;
};

#endif //_PIM_DATA_
