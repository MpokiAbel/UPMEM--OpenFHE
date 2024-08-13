#ifndef _PIM_MANAGER_
#define _PIM_MANAGER_

#include "DpuMemory.h"
#include <iostream>
#include "kernel.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

extern "C" {
#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
}

using namespace std;

class PimManager {
public:
  static PimManager *getPim(uint32_t nr_dpus, const std::string &profile = "") {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pim == nullptr) {
      pim = new PimManager(nr_dpus, profile);
    }
    return pim;
  }

  /**
   * copy_to_pim operation to send data to dpus.
   * copy can either be broadcast or rank transfer only i.e 0 or 1;
   * @param buf indicates the buffer to send to DPUs
   * @param size the size of the buffer, i,e elements in the buffer not in
   * bytes
   * @param type indicating which transfer is requested broadcast or rank
   * transfer default is broadcast
   * @param memory indicates the symbol name of where to tranfer the data
   * @return offset from which the array starts, since the data is divided
   * equally we assume that the offset is the same for all DPUs
   */

  void copy_to_pim(void *buf, uint32_t size, uint32_t offset, uint8_t type = 0,
                   const std::string &memory = DPU_MRAM_HEAP_POINTER_NAME);

  /**
   * copy_from_pim - function to copy data from the pim, all DPUs
   * @param buf buffer where the data will be put
   * @param size size of the buffer
   * @param offset offset where the data to read starts
   * @return
   */
  uint32_t copy_from_pim(uint64_t *buf, uint32_t size, uint32_t offset);

  /**
   loads the kernel, we separate the loading and starting the kernel,
   for usecases where there could be specific parameters for a specific kernel
   otherwise they could be merged.
 */
  void load_kernel(const std::string &bin) {
    DPU_ASSERT(dpu_load(set, bin.c_str(), NULL));
  }
  /**
    starts the kernel, to be checked for scenarios where ASYNCHRONOUS execution
    is to be prefered.
  */
  void start_kernel() { DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS)); }

  void display() {
    struct dpu_set_t dpu;
    DPU_FOREACH(set, dpu) { DPU_ASSERT(dpu_log_read(dpu, stdout)); }
  }

  void display_memory_status() const {
    for (size_t i = 0; i < chunks.size(); ++i) {
      std::cout << "Chunk " << i << ":\n";
      chunks[i]->display_status();
      std::cout << "\n";
    }
  }

  std::vector<std::pair<size_t, uint32_t>> allocate(size_t size);

  void deallocate(const std::vector<std::pair<size_t, uint32_t>> &addrs);

  uint32_t getNumDpus() { return nr_dpus; }

private:
  /**
   *This constructor loads the program to initialize the heap and other
   *structures. The initial plan is that to have only one kernel with
   *similar data structres for a wide usecase such that the kernel is loaded
   *only once. There for each invocation of a specif kernel i.e VA, GEMM etc
   *a paramenter is copied to the dpus instead of launching a whole new
   *kernel. Also we would like to keep the data in the MRAM when thea are
   *initialized for as long as needed.
   *@param count number of dpus to be launched
   *@param profile is the profile of the dpus to be launched
   */
  PimManager(uint32_t count, const std::string &profile = "") : nr_dpus(count) {
    DPU_ASSERT(dpu_alloc(count, profile.c_str(), &set));
    struct dpu_set_t dpu;
    load_kernel(BOOT);

    DPU_FOREACH(set, dpu) { chunks.push_back(new DpuMemory()); }
  }

  ~PimManager() {
    DPU_ASSERT(dpu_free(set));
    for (DpuMemory *chunk : chunks) {
      delete chunk;
    }
  }

  PimManager(const PimManager &) = delete;
  PimManager &operator=(const PimManager &) = delete;

  static PimManager *pim;
  static std::mutex mutex_;
  struct dpu_set_t set;
  uint32_t nr_dpus;
  std::vector<DpuMemory *> chunks;
  std::map<std::pair<size_t, uint32_t>,
           std::vector<std::pair<size_t, uint32_t>>>
      allocations;
};

#endif //_PIM_MANAGER_ 