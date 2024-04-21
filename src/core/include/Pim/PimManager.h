#ifndef PIMMANAGER_H
#define PIMMANAGER_H

#define ALLOCATE_ALL_DPUS 0

#include <cstddef>
#include <iostream>
#include <chrono>
#include <tuple>
#include <fstream>
#include <string>
#include <vector>
#include <stddef.h>
#include <stdint.h>
#include <cstdint>

using vector2D      = std::vector<std::vector<uint64_t>>;
using vector1D      = std::vector<uint64_t>;
using wrapperVector = std::vector<std::reference_wrapper<uint64_t>>;
using resultVector  = std::vector<wrapperVector>;

using string = std::string;

struct DPUData {
    vector2D buf1;
    vector2D buf2;
    vector2D mod;
    vector1D data_size_in_bytes;
};

class PimManager {
public:
    PimManager();
    PimManager(uint32_t num_dpus, const std::string& profile = "");
    ~PimManager();

    void load_binary(const std::string& binary);
    size_t get_dpu_count();

    template <typename Element>
    void copy_to_dpu(Element& a, const Element& b);
    // void copy_data_to_dpu(DPUData& dpuData);
    void execute();
    // void copy_from_dpus(size_t size, resultVector& results);
    template <typename Element>
    void copy_from_dpu(Element& a);

    template <typename Element>
    int run_on_pim(Element* a, const Element& b);

private:
    class PimManagerImpl;    // Forward declaration of the implementation class
    PimManagerImpl* PmImpl;  // Pointer to the implementation
};

#endif  // PIMMANAGER_H