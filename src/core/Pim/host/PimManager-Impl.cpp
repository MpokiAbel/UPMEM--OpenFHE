#include <dpu>

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

    void Copy_Data_To_Dpus(DPUData& dpuData) {
        size_t bytes = dpuData.data_size_in_bytes[0];

        // Move the data in parallel
        system.copy("mram_modulus", 0, dpuData.mod, sizeof(uint64_t));
        system.copy("data_copied_in_bytes", 0, dpuData.data_size_in_bytes, sizeof(uint64_t));
        system.copy(DPU_MRAM_HEAP_POINTER_NAME, 0, dpuData.buf1, bytes);
        system.copy(DPU_MRAM_HEAP_POINTER_NAME, bytes, dpuData.buf2, bytes);
    }

    // This functions distributes the data into the DPUs, tries as much as possible to distribute evenly
    template <typename Element>
    void Prepare_Data_For_Dpus(lbcrypto::DCRTPolyImpl<Element>& a, const lbcrypto::DCRTPolyImpl<Element>& b,
                               DPUData& dpuData, unsigned dpuSplit) {
        auto& mv1 = a.GetAllElements();
        auto& mv2 = b.GetAllElements();

        size_t m_vectorSize              = mv1.size();
        size_t current_split_index       = 0;
        size_t elements_in_current_split = 0;
        // size_t dpuNum                    = dpuSplit * m_vectorSize;
        size_t towerElementCount = mv1[0].GetValues().GetLength();
        size_t towerSplitCount   = towerElementCount / dpuSplit;
        size_t bytes             = towerSplitCount * sizeof(uint64_t);

        dpuData.buf1.resize(GetNumDpus(), vector1D(towerSplitCount));
        dpuData.buf2.resize(GetNumDpus(), vector1D(towerSplitCount));
        dpuData.mod.resize(GetNumDpus(), vector1D(1));
        dpuData.data_size_in_bytes = {bytes};

        for (size_t i = 0; i < m_vectorSize; ++i) {
            for (size_t j = 0; j < towerElementCount; ++j) {
                if (elements_in_current_split >= towerSplitCount) {
                    ++current_split_index;
                    elements_in_current_split = 0;
                }

                dpuData.buf1[current_split_index].push_back(mv1[i][j].ConvertToInt());
                dpuData.buf2[current_split_index].push_back(mv2[i][j].ConvertToInt());
                ++elements_in_current_split;
            }

            for (size_t k = i * dpuSplit; k < (i * dpuSplit) + dpuSplit; k++) {
                dpuData.mod[k][0] = mv1[i].GetValues().GetModulus().ConvertToInt();
            }
        }
    }

    /*This function copy the values from the DPUs to a container,
        the size of the container is basically the number of splits*/
    void Copy_From_Dpu(size_t size, vector2D& results) {
        size_t bytes = size * sizeof(uint64_t);
        try {
            system.copy(results, bytes, DPU_MRAM_HEAP_POINTER_NAME);
        }
        catch (const std::exception& e) {
            std::cerr << "Error copying data from DPU: " << e.what() << std::endl;
        }
    }

    /* 
        This function copies data from the DPUs and puts them back into the corresponding objects
    */
    template <typename Element>
    void fill_DCRTPoly(lbcrypto::DCRTPolyImpl<Element>& a, vector2D& results, unsigned int dpuSplit) {
        auto& mv = a.GetAllElements();

        size_t m_vectorSize              = mv.size();
        size_t current_split_index       = 0;
        size_t elements_in_current_split = 0;
        size_t dpuNum                    = dpuSplit * m_vectorSize;
        size_t towerElementCount         = mv[0].GetValues().GetLength();
        size_t towerSplitCount           = towerElementCount / dpuNum;

        /*    Set data on Each poly
             #pragma omp parallel for num_threads(lbcrypto::OpenFHEParallelControls.GetThreadLimit(m_vectorSize))
        */
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

    // This method is only called when we dont want to measure indiviadual operations i.e sending of data etc
    // It's a wrapper that performs all the operations including sending to DPU
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
            // Prepare_Data_For_Dpus(*a, b, 1);
            // // auto end = timeNow();
            // // LOGv("Copy data to DPUs takes: ", duration_ms(end - start));

            // // Execute the loaded program on all DPUs

            // // start = timeNow();
            Execute_On_Dpus();
            // // end = timeNow();
            // // LOGv("Execute operations takes: ", duration_ms(end - start));
            // system.log(outFile);

            // // start = timeNow();
            // fill_DCRTPoly(*a, 1);
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
void PimManager::Prepare_Data_For_Dpus(Element& a, const Element& b, DPUData& dpuData, unsigned split) {
    PmImpl->Prepare_Data_For_Dpus(a, b, dpuData, split);
}

void PimManager::Execute_On_Dpus() {
    PmImpl->Execute_On_Dpus();
}

void PimManager::Copy_From_Dpus(size_t size, vector2D& results) {
    PmImpl->Copy_From_Dpu(size, results);
}

void PimManager::Copy_Data_To_Dpus(DPUData& dpuData) {
    PmImpl->Copy_Data_To_Dpus(dpuData);
}

template <typename Element>
void PimManager::fill_DCRTPoly(Element& a, vector2D& results, unsigned split) {
    PmImpl->fill_DCRTPoly(a, results, split);
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
PimManager::Prepare_Data_For_Dpus<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>& a,
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>> const& b, DPUData& dpuData,
    unsigned split);

template void PimManager::fill_DCRTPoly<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>& a, vector2D& results,
    unsigned split);
