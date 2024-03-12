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

#ifndef DPU_BINARY
    #define DPU_BINARY "./src/core/dpu/mubintvecnat_dpu"
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

    static void Execute_On_Dpus(DpuSet& system) {
        system.exec();
    }

    void Initialize_Logging(std::ofstream& outFile) {
        outFile.open("logs.txt", std::ios::app);  // Log file opened in append mode
    }

    template <typename Element>
    void Copy_Data_To_Dpus(DpuSet& system, Element* a, Element b) {
        auto data_size             = a->GetLength() / GetNumDpus();  // Calculate data size per DPU
        auto size_to_copy_in_bytes = data_size * sizeof(uint64_t);   // size of data in bytes to copy
        auto data                  = GetData(*a, b, GetNumDpus());
        DvectorInt_64 data_to_copy{size_to_copy_in_bytes};  //  The values in bytes of the data copied

        // Copy data to DPUs
        system.copy("mram_modulus", std::get<2>(data));
        system.copy("data_copied", data_to_copy);

        for (size_t index = 0; index < GetNumDpus(); ++index) {
            system.dpus()[index]->copy(DPU_MRAM_HEAP_POINTER_NAME, 0, std::get<0>(data)[index], size_to_copy_in_bytes);
            system.dpus()[index]->copy(DPU_MRAM_HEAP_POINTER_NAME, size_to_copy_in_bytes, std::get<1>(data)[index],
                                       size_to_copy_in_bytes);
        }
    }

    template <typename Element>
    void Copy_Data_From_Dpus(DpuSet& system, Element* a) {
        auto data_size             = a->GetLength() / GetNumDpus();  // Calculate data size per DPU
        auto size_to_copy_in_bytes = data_size * sizeof(uint64_t);   // size of data in bytes to copy

        // Retrieve data to be processed
        TDvectorInt_64 data_results{GetNumDpus(), DvectorInt_64(data_size)};  // Container for DPU data results
        TDvectorInt_32 cycle_results{GetNumDpus(), DvectorInt_32(1)};         // Container for DPU time results
        TDvectorInt_32 clocks_per_sec{GetNumDpus(), DvectorInt_32(1)};  // Container for DPU clocks_per_sec results

        // Copy results from DPUs
        system.copy(data_results, size_to_copy_in_bytes, DPU_MRAM_HEAP_POINTER_NAME);
        system.copy(cycle_results, sizeof(uint32_t), "nb_cycles");
        system.copy(clocks_per_sec, sizeof(uint32_t), "CLOCKS_PER_SEC");
        // LOG("Done fetching from DPU !! \n");

        for (size_t i = 0; i < GetNumDpus(); i++) {
            LOGv("Time to run in the DPU ", (double)cycle_results[i].front() / clocks_per_sec[i].front());
        }

        // Post-processing
        SetData<Element, uint64_t>(a, data_results);
    }

    template <class Element, typename IntType>
    void SetData(Element* a, std::vector<std::vector<IntType>> results) {
        size_t index = 0;
        for (const auto& innerVec : results) {
            for (auto elem : innerVec) {
                (*a)[index].SetValue(std::to_string(elem));
                index++;
                if (index >= a->GetLength())
                    break;
            }
            if (index >= a->GetLength())
                break;
        }
    }

    template <typename Element>
    std::tuple<TDvectorInt_64, TDvectorInt_64, DvectorInt_64> GetData(Element& a, const Element& b, size_t n_buffers) {
        size_t size = a.GetLength() / n_buffers;

        TDvectorInt_64 a_data(n_buffers);
        TDvectorInt_64 b_data(n_buffers);
        DvectorInt_64 modulus{a.GetModulus().toNativeInt()};

        for (size_t i = 0; i < n_buffers; i++) {
            a_data[i].reserve(size);
            b_data[i].reserve(size);
            for (size_t j = 0; j < size; j++) {
                a_data[i].push_back(a[(i * size) + j].toNativeInt());
                b_data[i].push_back(b[(i * size) + j].toNativeInt());
            }
        }
        return std::make_tuple(a_data, b_data, modulus);
    }

public:
    PimManagerImpl(uint32_t num_dpus, const std::string& profile) : system(Allocate_Dpus(num_dpus, profile)) {}

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
            Load_Binary_To_Dpus(DPU_BINARY);

            Copy_Data_To_Dpus(system, a, b);
            // Execute the loaded program on all DPUs
            Execute_On_Dpus(system);
            // system.log(outFile);
            Copy_Data_From_Dpus(system, a);

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

PimManager::PimManager(uint32_t num_dpus, const std::string& profile) : PmImpl(new PimManagerImpl(num_dpus, profile)) {
    // Constructor implementation if needed
    std::cout << "DPUs created " << PmImpl->GetNumDpus() << std::endl;
}

PimManager::~PimManager() {
    std::cout << "DPUs deleted" << std::endl;
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
