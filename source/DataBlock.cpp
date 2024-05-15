#include <cstdio>
#include "DataBlock.h"

#include "dmntcht.h"
#include "res_macros.h"

DataBlock::DataBlock(const std::vector<u64>& offsets, size_t blockSize) {
  directAddress = 0;
  this->blockSize = blockSize;
  for (int i=0; i<offsets.size(); i++) {
    this->offsets.push_back(offsets[i]);
  }
}

DataBlock::~DataBlock() { }

Result DataBlock::ReadMemory(void *buffer) {
  u64 memoryBase = s_metadata.heap_extents.base; // m_metadata.main_nso_extents.base;
  return doWithCheatSession([this, memoryBase, buffer]() {
    u64 currentBase = memoryBase;
    for (std::vector<u64>::const_iterator it = offsets.begin(); it != offsets.end(); ++it) {
      if (std::next(it) != offsets.end()) {
        // Read pointer at each offset to get the next base address
        RETURN_IF_FAIL(dmntchtReadCheatProcessMemory(currentBase + *it, (void*)&currentBase, sizeof(u64)));
      } else {
        // For last offset, read the actual data into the buffer
        directAddress = currentBase + *it;
        RETURN_IF_FAIL(dmntchtReadCheatProcessMemory(directAddress, buffer, blockSize));
      }
    }

    // Indicate success
    return (unsigned)R_SUCCESS;
  });
}

Result DataBlock::ReadMemoryDirect(void *buffer) {
  // Have we computed the direct address yet?
  if (directAddress == 0) {
    return this->ReadMemory(buffer);
  }

  return doWithCheatSession([this, buffer]() {
    RETURN_IF_FAIL(dmntchtReadCheatProcessMemory(directAddress, buffer, blockSize));
    return (unsigned)R_SUCCESS;
  });
}
