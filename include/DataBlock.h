#pragma once

#include <vector>

#include "CheatSessionProvider.h"
#include "JSONMessage.h"

class DataBlock : public CheatSessionProvider {
 public:
  DataBlock(std::string clientKey, const uint64_t* offsets, size_t offsetCount, size_t blockSize,
            bool isDynamic = false);
  DataBlock(std::string clientKey, const ConfigMessage& streamMessage);
  ~DataBlock();

  void init(std::string clientKey, const uint64_t* offsets, size_t offsetCount, size_t blockSize,
            bool isDynamic);
  inline std::string getClientKey() const { return clientKey; }
  inline void ForceRecomputeOfDirectAddress() { directAddress = 0; }

  Result GetStatus(char*& metadataJson);
  Result ReadMemory(void* buffer);
  Result WriteMemory(void* buffer);

 protected:
  std::string clientKey;
  std::vector<uint64_t> offsets;
  uint64_t directAddress;
  size_t blockSize;
  bool isDynamic;

  friend class DataSession;
};
