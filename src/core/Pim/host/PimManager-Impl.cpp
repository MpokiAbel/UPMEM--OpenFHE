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

/* 
    Custome include files 
*/
#include "Pim/PimManager.h"
#include "math/hal/intnat/mubintvecnat.h"
#include "utils/debug.h"
#include "lattice/lat-hal.h"
#include "support-host.h"

#ifndef DPU_BINARY
    #define DPU_BINARY "./src/core/Pim/dpu/mubintvecnat_dpu"
#endif

#ifndef LOG
    #define LOG(x)     std::cout << x << std::endl
    #define LOGv(x, y) std::cout << x << ": " << y << std::endl
#endif

using namespace dpu;

using TDvectorInt_64 = std::vector<std::vector<uint64_t>>;
using TDvectorInt_32 = std::vector<std::vector<uint32_t>>;
using DvectorInt_64  = std::vector<uint64_t>;
using DvectorInt_32  = std::vector<uint32_t>;
using string         = std::string;

class PimManager::PimManagerImpl {
private:
    DpuSet system;

    static DpuSet Allocate_Dpus(uint32_t num_dpus, const string& profile = "") {
        return DpuSet::allocate(num_dpus, profile);
    }

    void Execute_On_Dpus() {
        system.exec();
    }

    void Initialize_Logging(std::ofstream& outFile) {
        outFile.open("logs.txt", std::ios::app);  // Log file opened in append mode
    }

    template <typename Element>
    void Copy_Data_To_Dpus(intnat::NativeVectorT<Element>* a, intnat::NativeVectorT<Element> b) {
        auto size = a->GetLength() / GetNumDpus();  // Calculate data size per DPU, this number always has to divide
        auto size_to_copy_in_bytes = size * sizeof(uint64_t);  // size of data in bytes to copy

        TDvectorInt_64 a_data(GetNumDpus());
        TDvectorInt_64 b_data(GetNumDpus());
        DvectorInt_64 modulus{a->GetModulus().ConvertToInt()};
        DvectorInt_64 data_to_copy{size_to_copy_in_bytes};  //  The values in bytes of the data copied

        for (size_t i = 0; i < GetNumDpus(); i++) {
            for (size_t j = 0; j < size; j++) {
                a_data[i].push_back((*a).at((i * size) + j).ConvertToInt());
                b_data[i].push_back(b.at((i * size) + j).ConvertToInt());
            }
        }

        // Copy data to DPUs
        system.copy("mram_modulus", modulus);
        system.copy("data_copied_in_bytes", data_to_copy);
        system.copy(DPU_MRAM_HEAP_POINTER_NAME, 0, a_data, size_to_copy_in_bytes);
        system.copy(DPU_MRAM_HEAP_POINTER_NAME, size_to_copy_in_bytes, b_data, size_to_copy_in_bytes);

        // LOGv("Data copied ", a_data[0].size());
    }

    template <typename Element>
    void Copy_Data_To_Dpus(lbcrypto::DCRTPolyImpl<Element>* a, const lbcrypto::DCRTPolyImpl<Element>& b) {
        auto& mv1 = a->GetAllElements();
        auto& mv2 = b.GetAllElements();

        TDvectorInt_64 buf1(GetNumDpus());
        TDvectorInt_64 buf2(GetNumDpus());
        TDvectorInt_64 mod(GetNumDpus());

        size_t m_vectorSize              = mv1.size();
        size_t current_split_index       = 0;
        size_t elements_in_current_split = 0;
        size_t dpuSplit                  = (GetNumDpus() / m_vectorSize);
        size_t towerElementCount         = mv1[0].GetValues().GetLength();
        size_t towerSplitCount           = towerElementCount / dpuSplit;

        // LOGv("Number of Splits ", dpuSplit);
        // LOGv("Tower Split Count", towerSplitCount);
#pragma omp parallel for num_threads(lbcrypto::OpenFHEParallelControls.GetThreadLimit(m_vectorSize))
        for (size_t i = 0; i < m_vectorSize; ++i) {
            for (size_t j = 0; j < towerElementCount; ++j) {
                if (elements_in_current_split >= towerSplitCount) {
                    ++current_split_index;
                    elements_in_current_split = 0;
                }

                buf1[current_split_index].push_back(mv1[i].at(j).ConvertToInt());
                buf2[current_split_index].push_back(mv2[i].at(j).ConvertToInt());
                ++elements_in_current_split;
            }

            for (size_t k = i * dpuSplit; k < (i * dpuSplit) + dpuSplit; k++) {
                mod[k].push_back(mv1[i].GetValues().GetModulus().ConvertToInt());
            }
        }

        size_t data_to_copy = towerSplitCount * sizeof(uint64_t);
        DvectorInt_64 data_size_in_bytes{data_to_copy};

        system.copy("mram_modulus", mod);
        system.copy("data_copied_in_bytes", data_size_in_bytes);
        system.copy(DPU_MRAM_HEAP_POINTER_NAME, 0, buf1, data_to_copy);
        system.copy(DPU_MRAM_HEAP_POINTER_NAME, data_to_copy, buf2);
    }

    template <typename Element>
    void Copy_Data_From_Dpus(intnat::NativeVectorT<Element>* a) {
        size_t size                  = a->GetLength() / GetNumDpus();  // Calculate data size per DPU
        size_t size_to_copy_in_bytes = size * sizeof(uint64_t);        // size of data in bytes to copy

        // Retrieve data to be processed
        TDvectorInt_64 results(GetNumDpus(), DvectorInt_64(size));  // Container for DPU data results

        // Copy results from DPUs
        system.copy(results, size_to_copy_in_bytes, DPU_MRAM_HEAP_POINTER_NAME);

        size_t index = 0;
        for (const auto& innerVec : results) {
            for (auto elem : innerVec) {
                // LOGv("Before ", (*a).at(index));
                (*a).at(index) = elem;
                // LOGv((*a).at(index), elem);
                index++;
                if (index > a->GetLength())
                    break;
            }
            if (index > a->GetLength())
                break;
        }
    }

    template <typename Element>
    void Copy_Data_From_Dpus(lbcrypto::DCRTPolyImpl<Element>* a) {
        auto& mv = a->GetAllElements();

        size_t m_vectorSize              = mv.size();
        size_t current_split_index       = 0;
        size_t elements_in_current_split = 0;
        size_t dpuSplit                  = (GetNumDpus() / m_vectorSize);
        size_t towerElementCount         = mv[0].GetValues().GetLength();
        size_t towerSplitCount           = towerElementCount / dpuSplit;
        size_t bytes                     = towerSplitCount * sizeof(uint64_t);

        TDvectorInt_64 results(GetNumDpus(), DvectorInt_64(towerSplitCount));  // Container for DPU data results
        system.copy(results, bytes, DPU_MRAM_HEAP_POINTER_NAME);

// Set data on Each poly
#pragma omp parallel for num_threads(lbcrypto::OpenFHEParallelControls.GetThreadLimit(m_vectorSize))
        for (size_t i = 0; i < m_vectorSize; i++) {
            for (size_t k = 0; k < towerElementCount; k++) {
                if (elements_in_current_split >= towerSplitCount) {
                    current_split_index++;
                    elements_in_current_split = 0;
                }
                mv[i].at(k) = results[current_split_index][elements_in_current_split];
            }
        }
    }

public:
    PimManagerImpl(uint32_t num_dpus, const std::string& profile) : system(Allocate_Dpus(num_dpus, profile)) {
        // LOG("DPUs Allocated");
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
            // auto end = timeNow();
            // LOGv("Load_Binary takes: ", duration_ms(end - start));

            // auto start = timeNow();
            Copy_Data_To_Dpus(a, b);
            // auto end = timeNow();
            // LOGv("Copy data to DPUs takes: ", duration_ms(end - start));

            // Execute the loaded program on all DPUs

            // start = timeNow();
            Execute_On_Dpus();
            // end = timeNow();
            // LOGv("Execute operations takes: ", duration_ms(end - start));
            system.log(outFile);

            // start = timeNow();
            Copy_Data_From_Dpus(a);
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

size_t PimManager::GetNumDpus() {
    return PmImpl->GetNumDpus();
}

template <typename Element>
int PimManager::Run_On_Pim(Element* a, const Element& b) {
    return PmImpl->Run_On_Pim(a, b);
}

// // Explicit instantiation of the template for the specific type you are using.
template int PimManager::Run_On_Pim(intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>* a,
                                    const intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>& b);

// template int PimManager::Run_On_Pim<lbcrypto::PolyImpl<intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>>>(
//     lbcrypto::PolyImpl<intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>>* a,
//     const lbcrypto::PolyImpl<intnat::NativeVectorT<intnat::NativeIntegerT<unsigned long>>>& b);

// template int PimManager::Run_On_Pim<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
//     lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>* a,
//     lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>> const& b);

template int PimManager::Run_On_Pim<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>* a,
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>> const& b);
