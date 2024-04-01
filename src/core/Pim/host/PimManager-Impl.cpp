#include <stddef.h>
#include <stdint.h>
#include <cstddef>
#include <cstdint>
#include <dpu>
#include <iostream>
#include <chrono>
#include <tuple>
#include <fstream>
#include <string>
#include <variant>
#include <vector>

/* 
    Custome include files 
*/
#include "Pim/PimManager.h"
#include "utils/debug.h"
#include "lattice/lat-hal.h"

#ifndef DPU_BINARY
    #define DPU_BINARY "./src/core/Pim/dpu/mubintvecnat_dpu"
#endif

#ifndef LOG
    #define LOG(x)     std::cout << x << std::endl
    #define LOGv(x, y) std::cout << x << ": " << y << std::endl
#endif

using namespace dpu;

using vector2D = std::vector<std::vector<uint64_t>>;
using vector1D = std::vector<uint64_t>;
using string   = std::string;

// struct MyData {
//     string name;
//     uint64_t index;
//     std::variant<vector1D, vector2D> data;
//     uint64_t size;

//     // Constructor to initialize the members
//     MyData(const string& n, uint64_t i, const vector1D& m, uint64_t v) : name(n), index(i), data(m), size(v) {}

//     MyData(const string& n, uint64_t i, const vector2D& m, uint64_t v) : name(n), index(i), data(m), size(v) {}
// };

class PimManager::PimManagerImpl {
private:
    DpuSet system;
    static DpuSet Allocate_Dpus(uint32_t num_dpus, const string& profile = "") {
        return num_dpus == ALLOCATE_ALL_DPUS ? DpuSet::allocate(ALLOCATE_ALL, profile) :
                                               DpuSet::allocate(num_dpus, profile);
    }

public:
    void Execute_On_Dpus() {
        system.exec();
    }

    void Initialize_Logging(std::ofstream& outFile) {
        outFile.open("logs.txt", std::ios::app);  // Log file opened in append mode
    }

    // void sendData(std::vector<MyData>& data) {
    //     for (auto& i : data) {
    //         system.copy(i.name, i.index, i.data, i.size);
    //     }
    // }

    template <typename Element>
    void Copy_Data_To_Dpus(lbcrypto::DCRTPolyImpl<Element>* a, const lbcrypto::DCRTPolyImpl<Element>& b,
                           unsigned dpuSplit) {
        auto& mv1 = a->GetAllElements();
        auto& mv2 = b.GetAllElements();

        size_t m_vectorSize              = mv1.size();
        size_t current_split_index       = 0;
        size_t elements_in_current_split = 0;
        // size_t dpuNum                    = dpuSplit * m_vectorSize;
        size_t towerElementCount = mv1[0].GetValues().GetLength();
        size_t towerSplitCount   = towerElementCount / dpuSplit;
        vector2D buf1(GetNumDpus(), vector1D(towerSplitCount));
        vector2D buf2(GetNumDpus(), vector1D(towerSplitCount));
        vector2D mod(GetNumDpus(), vector1D(1));

        // #pragma omp parallel for num_threads(OpenFHEParallelControls.GetThreadLimit(m_vectorSize))
        for (size_t i = 0; i < m_vectorSize; ++i) {
            for (size_t j = 0; j < towerElementCount; ++j) {
                if (elements_in_current_split >= towerSplitCount) {
                    ++current_split_index;
                    elements_in_current_split = 0;
                }

                buf1[current_split_index].push_back(mv1[i][j].ConvertToInt());
                buf2[current_split_index].push_back(mv2[i][j].ConvertToInt());
                ++elements_in_current_split;
            }

            for (size_t k = i * dpuSplit; k < (i * dpuSplit) + dpuSplit; k++) {
                mod[k][0] = mv1[i].GetValues().GetModulus().ConvertToInt();
            }
        }

        size_t data_to_copy = towerSplitCount * sizeof(uint64_t);
        vector1D data_size_in_bytes{data_to_copy};

        // Move the data in parallel
        system.copy("mram_modulus", mod);
        system.copy("data_copied_in_bytes", data_size_in_bytes);
        system.copy(DPU_MRAM_HEAP_POINTER_NAME, 0, buf1, data_to_copy);
        system.copy(DPU_MRAM_HEAP_POINTER_NAME, data_to_copy, buf2, data_to_copy);
    }

    template <typename Element>
    void Copy_Data_From_Dpus(lbcrypto::DCRTPolyImpl<Element>* a, unsigned int dpuSplit) {
        auto& mv = a->GetAllElements();

        size_t m_vectorSize              = mv.size();
        size_t current_split_index       = 0;
        size_t elements_in_current_split = 0;
        size_t dpuNum                    = dpuSplit * m_vectorSize;
        size_t towerElementCount         = mv[0].GetValues().GetLength();
        size_t towerSplitCount           = towerElementCount / dpuNum;
        size_t bytes                     = towerSplitCount * sizeof(uint64_t);

        vector2D results(GetNumDpus(), vector1D(towerSplitCount));  // Container for DPU data results
        system.copy(results, bytes, DPU_MRAM_HEAP_POINTER_NAME);

        // Set data on Each poly
        // #pragma omp parallel for num_threads(lbcrypto::OpenFHEParallelControls.GetThreadLimit(m_vectorSize))
        for (size_t i = 0; i < m_vectorSize; i++) {
            for (size_t k = 0; k < towerElementCount; k++) {
                if (elements_in_current_split >= towerSplitCount) {
                    current_split_index++;
                    elements_in_current_split = 0;
                }
                mv[i][k] = results[current_split_index][elements_in_current_split];
            }
        }
    }

    PimManagerImpl(uint32_t num_dpus, const std::string& profile) : system(Allocate_Dpus(num_dpus, profile)) {
        LOGv("DPUs Allocated", GetNumDpus());
    }

    void Load_Binary_To_Dpus(const std::string& binary) {
        system.load(binary);
    }
    size_t GetNumDpus() {
        return system.dpus().size();
    }

    template <typename Element>
    int Run_On_Pim(Element* a, const Element& b) {
        int ret = 0;  // Variable to hold the return value of this function, indicating success or failure
        std::ofstream outFile;
        Initialize_Logging(outFile);

        try {
            // LOG("\nEntry to run dpu");

            // Load binary to DPUs
            // auto start = timeNow();
            Load_Binary_To_Dpus(DPU_BINARY);
            // // auto end = timeNow();
            // // LOGv("Load_Binary takes: ", duration_ms(end - start));

            // // auto start = timeNow();
            Copy_Data_To_Dpus(a, b, 1);
            // // auto end = timeNow();
            // // LOGv("Copy data to DPUs takes: ", duration_ms(end - start));

            // // Execute the loaded program on all DPUs

            // // start = timeNow();
            Execute_On_Dpus();
            // // end = timeNow();
            // // LOGv("Execute operations takes: ", duration_ms(end - start));
            // system.log(outFile);

            // // start = timeNow();
            Copy_Data_From_Dpus(a, 1);
            // end = timeNow();
            // LOGv("Copy data from DPUs takes: ", duration_ms(end - start));

            ret = 1;
        }
        catch (const DpuError& e) {
            std::cerr << e.what() << std::endl;
        }

        if (outFile.is_open()) {
            outFile.close();
        }

        return ret;
    }
};

PimManager::PimManager() : PmImpl(nullptr) {}
PimManager::PimManager(uint32_t num_dpus, const std::string& profile) : PmImpl(new PimManagerImpl(num_dpus, profile)) {
    // Constructor implementation if needed
    // std::cout << "DPUs created " << PmImpl->GetNumDpus() << std::endl;
}

PimManager::~PimManager() {
    // std::cout << "DPUs deleted " << PmImpl->GetNumDpus() << std::endl;
    delete PmImpl;
}

void PimManager::Load_Binary_To_Dpus(const std::string& binary) {
    PmImpl->Load_Binary_To_Dpus(binary);
}

template <typename Element>
void PimManager::Copy_Data_To_Dpus(Element* a, const Element& b, unsigned split) {
    PmImpl->Copy_Data_To_Dpus(a, b, split);
}

void PimManager::Execute_On_Dpus() {
    PmImpl->Execute_On_Dpus();
}

template <typename Element>
void PimManager::Copy_Data_From_Dpus(Element* a, unsigned split) {
    PmImpl->Copy_Data_From_Dpus(a, split);
}

size_t PimManager::GetNumDpus() {
    return PmImpl->GetNumDpus();
}

template <typename Element>
int PimManager::Run_On_Pim(Element* a, const Element& b) {
    return PmImpl->Run_On_Pim(a, b);
}

template int PimManager::Run_On_Pim<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>* a,
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>> const& b);

template void
PimManager::Copy_Data_To_Dpus<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>* a,
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>> const& b, unsigned split);

template void
PimManager::Copy_Data_From_Dpus<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>* a, unsigned split);
