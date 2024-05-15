#pragma once

#include <types.h>
#include <vector>

#include "CheatSessionProvider.h"

class DataBlock : public CheatSessionProvider {
public:
    DataBlock(const std::vector<u64>& offsets, size_t blockSize);
    ~DataBlock();

    inline void ForceRecomputeOfDirectAddress() { directAddress = 0; }
    Result ReadMemory(void *buffer);
    Result ReadMemoryDirect(void *buffer);

protected:
    std::vector<u64> offsets;
    u64 directAddress;
    size_t blockSize;
};
