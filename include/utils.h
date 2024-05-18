#pragma once

#include <string>
#include <vector>

#include <rapidjson/document.h>
#include <switch.h>

using namespace rapidjson;

enum eRequestDataType {
  eRequestDataType_f64,
  eRequestDataType_f32,

  eRequestDataType_i64,
  eRequestDataType_i32,
  eRequestDataType_i16,

  eRequestDataType_u64,
  eRequestDataType_u32,
  eRequestDataType_u16,
  eRequestDataType_u8,

  eRequestDataType_Invalid
};

std::string convertByteArrayToHex(u8* bytes, size_t size);
std::string convertNumToHexString(u64 num, int width = 16, bool withPrefix = false);

int sizeFromType(eRequestDataType dataType);
float interpretAsFloat(u8* buffer);
double interpretAsDouble(u8* buffer);

Value interpretDataType(eRequestDataType dataType, u8* buffer, u64 count,
                        MemoryPoolAllocator<CrtAllocator>& allocator);
