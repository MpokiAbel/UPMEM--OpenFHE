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

using vector2D = std::vector<std::vector<uint64_t>>;
using vector1D = std::vector<uint64_t>;
using string   = std::string;

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

    void Load_Binary_To_Dpus(const std::string& binary);
    size_t GetNumDpus();

    template <typename Element>
    void Prepare_Data_For_Dpus(Element& a, const Element& b, DPUData& dpuData, unsigned split = 1);
    void Copy_Data_To_Dpus(DPUData& dpuData);
    void Execute_On_Dpus();
    void Copy_From_Dpus(size_t size, vector2D& results);
    template <typename Element>
    void fill_DCRTPoly(Element& a, vector2D& results, unsigned split = 1);

    template <typename Element>
    int Run_On_Pim(Element* a, const Element& b);

private:
    class PimManagerImpl;    // Forward declaration of the implementation class
    PimManagerImpl* PmImpl;  // Pointer to the implementation
};

#endif  // PIMMANAGER_H