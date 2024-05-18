#include "DataBlock.h"
#include <cstdio>

#include "dmntcht.h"
#include "res_macros.h"

// Turn extents struct into JSON
#define MAKE_EXTENTS_JSON(name, extents)                                                     \
  MKJSON_JSON_FREE, (name),                                                                  \
      mkjson(MKJSON_OBJ, 2, MKJSON_STRING, "base",                                           \
             convertNumToHexString((extents).base, 10, true).c_str(), MKJSON_STRING, "size", \
             convertNumToHexString((extents).size, 10, true).c_str())

#define MAKE_BOUNDS_JSON(name, extents)                                                     \
  MKJSON_JSON_FREE, (name),                                                                 \
      mkjson(MKJSON_OBJ, 2, MKJSON_STRING, "start",                                         \
             convertNumToHexString((extents).base, 10, true).c_str(), MKJSON_STRING, "end", \
             convertNumToHexString((extents).base + (extents).size, 10, true).c_str())

DataBlock::DataBlock(std::string clientKey, const uint64_t* offsets, size_t offsetCount,
                     size_t blockSize) {
  init(clientKey, offsets, offsetCount, blockSize);
}

DataBlock::DataBlock(std::string clientKey, const ConfigMessage& streamMessage) {
  uint64_t blockSize = sizeFromType(streamMessage.dataType) * streamMessage.dataCount;
  init(clientKey, streamMessage.offsets, streamMessage.offsetCount, blockSize);
}

DataBlock::~DataBlock() { }

void DataBlock::init(std::string clientKey, const uint64_t* offsets, size_t offsetCount,
                     size_t blockSize) {
  directAddress = 0;
  this->clientKey = clientKey;
  this->blockSize = blockSize;

  for (size_t i = 0; i < offsetCount; i++) {
    this->offsets.push_back(offsets[i]);
  }
}

Result DataBlock::GetStatus(char*& metadataJson) {
  return doWithCheatSession([this, &metadataJson]() {
    if (!s_hasMetadata) {
      return MAKERESULT(module_syscricket, syscricket_badMetadata);
    }

    metadataJson = mkjson(
        MKJSON_OBJ, 10, MKJSON_STRING, "messageType", "status", MKJSON_INT, "processId",
        s_metadata.process_id, MKJSON_STRING, "titleId",
        convertNumToHexString(s_metadata.title_id).c_str(), MKJSON_STRING, "mainNsoBuildId",
        convertByteArrayToHex(s_metadata.main_nso_build_id, 0x20).c_str(),
        MAKE_BOUNDS_JSON("main", this->s_metadata.main_nso_extents),
        MAKE_BOUNDS_JSON("heap", this->s_metadata.alias_extents),
        MAKE_EXTENTS_JSON("mainNsoExtents", this->s_metadata.main_nso_extents),
        MAKE_EXTENTS_JSON("aliasExtents", this->s_metadata.alias_extents),
        MAKE_EXTENTS_JSON("heapExtents", this->s_metadata.heap_extents),
        MAKE_EXTENTS_JSON("addressSpaceExtents", this->s_metadata.address_space_extents));
    return R_SUCCESS;
  });
}

Result DataBlock::ReadMemory(void* buffer) {
  uint64_t memoryBase = s_metadata.main_nso_extents.base;  // s_metadata.heap_extents.base;
  return doWithCheatSession([this, memoryBase, buffer]() {
    uint64_t currentBase = memoryBase;
    for (std::vector<uint64_t>::const_iterator it = offsets.begin(); it != offsets.end(); ++it) {
      if (std::next(it) != offsets.end()) {
        // Read pointer at each offset to get the next base address
        Result result = dmntchtReadCheatProcessMemory(currentBase + *it, (void*)&currentBase,
                                                      sizeof(uint64_t));
        if (R_FAILED(result)) {
          fprintf(stderr, "DataBlock: indirect read of %#018lx failed (%u, %u)\n",
                  currentBase + *it, R_MODULE(result), R_DESCRIPTION(result));
          return result;
        }
      } else {
        // For last offset, read the actual data into the buffer
        directAddress = currentBase + *it;
        Result result = dmntchtReadCheatProcessMemory(directAddress, buffer, blockSize);
        if (R_FAILED(result)) {
          fprintf(stderr, "DataBlock: indirect read of %#018lx failed (%u, %u)\n", directAddress,
                  R_MODULE(result), R_DESCRIPTION(result));
          return result;
        }
      }
    }

    return (unsigned)R_SUCCESS;
  });
}

Result DataBlock::ReadMemoryDirect(void* buffer) {
  // Have we computed the direct address yet?
  if (directAddress == 0) {
    return this->ReadMemory(buffer);
  }

  return doWithCheatSession([this, buffer]() {
    Result result = dmntchtReadCheatProcessMemory(directAddress, buffer, blockSize);
    if (R_FAILED(result)) {
      fprintf(stderr, "DataBlock: direct read of %#018lx failed (%u, %u)\n", directAddress,
              R_MODULE(result), R_DESCRIPTION(result));
      return result;
    }
    return (unsigned)R_SUCCESS;
  });
}
