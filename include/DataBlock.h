#pragma once

#include <types.h>
#include <vector>

#include "CheatSessionProvider.h"

class DataBlock : public CheatSessionProvider {
 public:
  DataBlock(u64 clientKey, const u64* offsets, size_t offsetCount, size_t blockSize);
  ~DataBlock();

  inline u64 getClientKey() const { return clientKey; }
  inline void ForceRecomputeOfDirectAddress() { directAddress = 0; }

  Result ReadMemory(void* buffer);
  Result ReadMemoryDirect(void* buffer);

 protected:
  u64 clientKey;
  std::vector<u64> offsets;
  u64 directAddress;
  size_t blockSize;
};
