#include "pim/PimManager.h"
#include "pim/kernel.h"
#include <cstdint>

void PimManager::copy_to_pim(void *buf, uint32_t size, uint32_t offset,
                             uint8_t type, const std::string &memory) {
  struct dpu_set_t dpu;
  uint32_t each_dpu = 0;
  uint32_t size_pr_dpu = 0;
  uint32_t size_pr_dpu_bytes = 0;
  uint64_t *uintPtr = static_cast<uint64_t *>(buf);

  switch (type) {
  case 0:
    // Here i would need to div and round up and append zeros to the end of the
    // array
    size_pr_dpu = DIV(size, nr_dpus);
    size_pr_dpu_bytes = size_pr_dpu * sizeof(uint64_t);

    DPU_FOREACH(set, dpu, each_dpu) {
      DPU_ASSERT(dpu_prepare_xfer(dpu, &uintPtr[each_dpu * size_pr_dpu]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, memory.c_str(), offset,
                             size_pr_dpu_bytes, DPU_XFER_DEFAULT));
    break;

  case 1:
    // std::cout<< memory << size<<std::endl;
    DPU_ASSERT(
        dpu_broadcast_to(set, memory.c_str(), 0, buf, size, DPU_XFER_DEFAULT));
    break;

  default:
    std::cout << "Not Available copy option" << std::endl;
    break;
  }
}

uint32_t PimManager::copy_from_pim(uint64_t *buf, uint32_t size,
                                   uint32_t offset) {
  struct dpu_set_t dpu;
  uint32_t each_dpu = 0;
  uint32_t size_pr_dpu = 0;
  uint32_t size_pr_dpu_bytes = 0;
  size_pr_dpu = DIV(size, nr_dpus);
  size_pr_dpu_bytes = size_pr_dpu * sizeof(uint64_t);

  DPU_FOREACH(set, dpu, each_dpu) {
    DPU_ASSERT(dpu_prepare_xfer(dpu, &buf[each_dpu * size_pr_dpu]));
  }
  DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME,
                           offset, size_pr_dpu_bytes, DPU_XFER_DEFAULT));
  return 0;
}

std::vector<std::pair<size_t, uint32_t>> PimManager::allocate(size_t size) {
  std::vector<std::pair<size_t, uint32_t>> allocated_addrs;
  size_t size_per_chunk =
      DIVROUNDUP(size, PimManager::nr_dpus); // Round up to split evenly

  for (size_t i = 0; i < chunks.size(); ++i) {
    int32_t addr = chunks[i]->allocate(size_per_chunk);
    if (addr != -1) {
      allocated_addrs.push_back({i, addr});
      size -= size_per_chunk;
      if (size <= 0)
        break;
    } else {
      std::cerr << "Allocation failed in one of the chunks.\n";
      deallocate(allocated_addrs); // Rollback allocation if any chunk fails
      return {};
    }
  }

  if (size > 0) {
    std::cerr << "Insufficient memory across all chunks.\n";
    deallocate(allocated_addrs); // Rollback allocation if not enough memory
    return {};
  }

  allocations[allocated_addrs[0]] = allocated_addrs;
  return allocated_addrs;
}

void PimManager::deallocate(
    const std::vector<std::pair<size_t, uint32_t>> &addrs) {
  for (const auto &[chunk_index, addr] : addrs) {
    chunks[chunk_index]->deallocate(addr);
  }
  if (!addrs.empty() && allocations.find(addrs[0]) != allocations.end()) {
    allocations.erase(addrs[0]);
  }
}


PimManager *PimManager::pim = nullptr;
std::mutex PimManager::mutex_;