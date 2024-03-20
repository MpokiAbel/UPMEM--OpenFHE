#ifndef HOST_H
#define HOST_H

#include <cstdint>
#include <stdint.h>
#include <stddef.h>
#include <vector>

struct PolyDpu {
    uint64_t modulus;
    uint64_t size;
    std::vector<uint64_t> vec;  // Use a vector for dynamic array of uint64_t
};

struct DCRTPolyDpu {
    uint64_t size;
    std::vector<PolyDpu> poly;  // Use a vector of PolyParams
};

#endif  // HOST_H
