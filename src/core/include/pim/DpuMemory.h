#include "common.h"
#include <cstdint>
#include <iostream>
#include <map>
#include <sys/types.h>
#include <vector>

#define CHUNK_SIZE (64 * 1024 * 1024) // 64 MB
#define BLOCK_SIZE (4 * 1024)         // 4k blocks within each chunk

class DpuMemory {
public:
  DpuMemory() : free_blocks(TOTAL_BLOCKS, true) {}

  ~DpuMemory() {}

  int32_t allocate(size_t size) {
    if (size > CHUNK_SIZE) {
      return -1;
    }
    size_t blocks_needed = DIVROUNDUP(size, BLOCK_SIZE);
    int32_t start_block = find_free_blocks(blocks_needed);
    if (start_block == -1) {
      return -1;
    }
    mark_blocks(start_block, blocks_needed, false);
    uint32_t addr = start_block * BLOCK_SIZE;
    allocations[addr] = blocks_needed;
    return addr;
  }

  void deallocate(uint32_t addr) {
    if (allocations.find(addr) != allocations.end()) {
      size_t start_block = DIV(addr, BLOCK_SIZE);
      size_t blocks_to_free = allocations[addr];
      mark_blocks(start_block, blocks_to_free, true);
      allocations.erase(addr);
    } else {
      std::cerr << "Invalid address to deallocate.\n";
    }
  }

  void display_status() const {
    const std::string red("\033[31m");
    const std::string reset("\033[0m");

    for (size_t i = 0; i < TOTAL_BLOCKS; ++i) {
      if (free_blocks[i]) {
        std::cout << "Free ";
      } else {
        std::cout << red << "Aloc" << reset << " ";
      }
      if ((i + 1) % 16 == 0)
        std::cout << "\n";
    }
  }

private:
  static const size_t TOTAL_BLOCKS = DIV(CHUNK_SIZE, BLOCK_SIZE);
  std::vector<bool> free_blocks;
  std::map<uint32_t, size_t> allocations;

  size_t find_free_blocks(size_t blocks_needed) {
    size_t consecutive_free = 0;
    for (size_t i = 0; i < TOTAL_BLOCKS; ++i) {
      if (free_blocks[i]) {
        ++consecutive_free;
        if (consecutive_free == blocks_needed) {
          return i - blocks_needed + 1;
        }
      } else {
        consecutive_free = 0;
      }
    }
    return -1;
  }

  void mark_blocks(size_t start_block, size_t blocks_needed, bool free) {
    for (size_t i = start_block; i < start_block + blocks_needed; ++i) {
      free_blocks[i] = free;
    }
  }
};