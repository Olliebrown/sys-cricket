#include "DataBlock.h"
#include <cstdio>

#include "dmntcht.h"
#include "res_macros.h"

DataBlock::DataBlock(uint64_t clientKey, const uint64_t* offsets, size_t offsetCount,
                     size_t blockSize) {
  directAddress = 0;
  this->clientKey = clientKey;
  this->blockSize = blockSize;

  for (size_t i = 0; i < offsetCount; i++) {
    this->offsets.push_back(offsets[i]);
  }
}

DataBlock::DataBlock(uint64_t clientKey, const ConfigMessage& streamMessage) {
  DataBlock(clientKey, streamMessage.offsets, streamMessage.offsetCount, streamMessage.blockSize);
}

DataBlock::~DataBlock() { }

Result DataBlock::ReadMemory(void* buffer) {
  uint64_t memoryBase = s_metadata.heap_extents.base;  // m_metadata.main_nso_extents.base;
  return doWithCheatSession([this, memoryBase, buffer]() {
    uint64_t currentBase = memoryBase;
    for (std::vector<uint64_t>::const_iterator it = offsets.begin(); it != offsets.end(); ++it) {
      if (std::next(it) != offsets.end()) {
        // Read pointer at each offset to get the next base address
        RETURN_IF_FAIL(dmntchtReadCheatProcessMemory(currentBase + *it, (void*)&currentBase,
                                                     sizeof(uint64_t)));
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

Result DataBlock::ReadMemoryDirect(void* buffer) {
  // Have we computed the direct address yet?
  if (directAddress == 0) {
    return this->ReadMemory(buffer);
  }

  return doWithCheatSession([this, buffer]() {
    RETURN_IF_FAIL(dmntchtReadCheatProcessMemory(directAddress, buffer, blockSize));
    return (unsigned)R_SUCCESS;
  });
}
