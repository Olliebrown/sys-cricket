#pragma once

#include <vector>

#include "CheatSessionProvider.h"
#include "JSONMessage.h"

class DataBlock : public CheatSessionProvider {
 public:
  DataBlock(uint64_t clientKey, const uint64_t* offsets, size_t offsetCount, size_t blockSize);
  DataBlock(uint64_t clientKey, const ConfigMessage& streamMessage);
  ~DataBlock();

  inline uint64_t getClientKey() const { return clientKey; }
  inline void ForceRecomputeOfDirectAddress() { directAddress = 0; }

  Result ReadMemory(void* buffer);
  Result ReadMemoryDirect(void* buffer);

 protected:
  uint64_t clientKey;
  std::vector<uint64_t> offsets;
  uint64_t directAddress;
  size_t blockSize;
};
