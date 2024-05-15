#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include "Pim/PimManager.h"
#include "utils/debug.h"
#include "lattice/lat-hal.h"
extern "C" {
#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
}

#ifndef DPU_BINARY
    #define DPU_BINARY "./src/core/Pim/dpu/mubintvecnat_dpu"
#endif

#ifndef LOG
    #define LOG(x)     std::cout << x << std::endl
    #define LOGv(x, y) std::cout << x << ": " << y << std::endl
#endif

// using namespace dpu;

class PimManager::PimManagerImpl {
private:
    struct dpu_set_t dpu_set;
    uint32_t nr_dpus;
    void allocate(uint32_t num_dpus, const string& profile = "") {
        if (num_dpus == ALLOCATE_ALL_DPUS)
            DPU_ASSERT(dpu_alloc(DPU_ALLOCATE_ALL, profile.c_str(), &dpu_set));
        else
            DPU_ASSERT(dpu_alloc(num_dpus, profile.c_str(), &dpu_set));

        DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_dpus));
    }

public:
    void execute() {
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    }

    void log_on_file(std::ofstream& outFile) {
        outFile.open("logs.txt", std::ios::app);  // Log file opened in append mode
    }

    template <typename Element>
    void copy_to_dpu(lbcrypto::DCRTPolyImpl<Element>& a, const lbcrypto::DCRTPolyImpl<Element>& b) {
        auto& mv1                = a.GetAllElements();
        auto& mv2                = b.GetAllElements();
        size_t dpuNum            = get_dpu_count() / mv1.size();
        size_t towerElementCount = a.GetAllElements()[0].GetValues().GetLength();
        size_t towerSplitCount   = towerElementCount / dpuNum;

        uint64_t bytes = towerSplitCount * sizeof(uint64_t);
        struct dpu_set_t dpu;
        size_t i = 0, j = 0, k = 0;
        uint64_t* buf;

        DPU_FOREACH(dpu_set, dpu, i) {
            if (i >= dpuNum * (j + 1))
                j++;
            k   = i - dpuNum * j;
            buf = reinterpret_cast<uint64_t*>(&mv1[j][k * towerSplitCount]);
            DPU_ASSERT(dpu_prepare_xfer(dpu, (void*)buf));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, bytes, DPU_XFER_DEFAULT));

        j = 0, i = 0;
        const uint64_t* buf2;
        DPU_FOREACH(dpu_set, dpu, i) {
            if (i >= dpuNum * (j + 1))
                j++;
            k    = i - dpuNum * j;
            buf2 = reinterpret_cast<const uint64_t*>(&mv2[j][k * towerSplitCount]);
            DPU_ASSERT(dpu_prepare_xfer(dpu, (void*)buf2));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, bytes, bytes, DPU_XFER_DEFAULT));

        j = 0, i = 0;
        uint64_t modulus;
        DPU_FOREACH(dpu_set, dpu, i) {
            if (i >= dpuNum * (j + 1))
                j++;
            modulus = mv1[j].GetValues().GetModulus().ConvertToInt();
            DPU_ASSERT(dpu_prepare_xfer(dpu, (void*)&modulus));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "mram_modulus", 0, sizeof(uint64_t), DPU_XFER_DEFAULT));

        DPU_ASSERT(
            dpu_broadcast_to(dpu_set, "data_copied_in_bytes", 0, (void*)&bytes, sizeof(uint64_t), DPU_XFER_DEFAULT));
    }

    template <typename Element>
    void copy_from_dpu(lbcrypto::DCRTPolyImpl<Element>& a) {
        auto& mv = a.GetAllElements();

        size_t dpuNum            = get_dpu_count() / mv.size();
        size_t towerElementCount = a.GetAllElements()[0].GetValues().GetLength();
        size_t towerSplitCount   = towerElementCount / dpuNum;

        size_t bytes = towerSplitCount * sizeof(uint64_t);
        struct dpu_set_t dpu;
        size_t i = 0, j = 0, k = 0;
        uint64_t* buf;

        DPU_FOREACH(dpu_set, dpu, i) {
            if (i >= dpuNum * (j + 1)) {
                j++;
            }
            k   = i - dpuNum * j;
            buf = reinterpret_cast<uint64_t*>(&mv[j][k * towerSplitCount]);
            DPU_ASSERT(dpu_prepare_xfer(dpu, (void*)buf));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, bytes, DPU_XFER_DEFAULT));
    }

    PimManagerImpl(uint32_t num_dpus, const std::string& profile) {
        allocate(num_dpus, profile);
        LOGv("DPUs Allocated", get_dpu_count());
    }

    void load_binary(const std::string& binary) {
        DPU_ASSERT(dpu_load(dpu_set, binary.c_str(), NULL));
    }

    size_t get_dpu_count() {
        return nr_dpus;
    }

    template <typename Element>
    int run_on_pim(Element* a, const Element& b) {
        int ret = 0;
        return ret;
    }
};

PimManager::PimManager() : PmImpl(nullptr) {}
PimManager::PimManager(uint32_t num_dpus, const std::string& profile) : PmImpl(new PimManagerImpl(num_dpus, profile)) {
    // Constructor implementation if needed
    // std::cout << "DPUs created " << PmImpl->get_dpu_count() << std::endl;
}

PimManager::~PimManager() {
    // std::cout << "DPUs deleted " << PmImpl->get_dpu_count() << std::endl;
    delete PmImpl;
}

void PimManager::load_binary(const std::string& binary) {
    PmImpl->load_binary(binary);
}

template <typename Element>
void PimManager::copy_to_dpu(Element& a, const Element& b) {
    PmImpl->copy_to_dpu(a, b);
}

void PimManager::execute() {
    PmImpl->execute();
}

template <typename Element>
void PimManager::copy_from_dpu(Element& a) {
    PmImpl->copy_from_dpu(a);
}

size_t PimManager::get_dpu_count() {
    return PmImpl->get_dpu_count();
}

template <typename Element>
int PimManager::run_on_pim(Element* a, const Element& b) {
    return PmImpl->run_on_pim(a, b);
}

template int PimManager::run_on_pim<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>* a,
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>> const& b);

template void PimManager::copy_to_dpu<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>& a,
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>> const& b);

template void PimManager::copy_from_dpu<lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>>(
    lbcrypto::DCRTPolyImpl<bigintdyn::mubintvec<bigintdyn::ubint<unsigned long>>>& a);
