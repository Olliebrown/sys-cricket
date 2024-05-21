#include "utils.h"

#include <cstring>
#include <iomanip>
#include <vector>

using namespace rapidjson;

std::string convertByteArrayToHex(u8* bytes, size_t size) {
  std::stringstream stream;

  stream << std::setfill('0') << std::hex;
  for (u64 i = 0; i < size; i++) {
    stream << std::setw(2) << static_cast<int>(bytes[i]);
  }

  return stream.str();
}

std::string convertNumToHexString(u64 num, int width, bool withPrefix) {
  std::stringstream hex;
  if (withPrefix) {
    hex << "0x";
  }
  hex << std::setfill('0') << std::setw(width) << std::hex << num;
  return hex.str();
}

int sizeFromType(eRequestDataType dataType) {
  switch (dataType) {
    case eRequestDataType_f64:
    case eRequestDataType_i64:
    case eRequestDataType_u64:
      return 8;

    case eRequestDataType_f32:
    case eRequestDataType_i32:
    case eRequestDataType_u32:
      return 4;

    case eRequestDataType_i16:
    case eRequestDataType_u16:
      return 2;

    case eRequestDataType_u8:
      return 1;

    default:
      return 0;
  }
}

float interpretAsFloat(u8* buffer) {
  u8 swapped[4] = { buffer[0], buffer[1], buffer[2], buffer[3] };

  float result;
  std::memcpy(&result, swapped, 4);
  return result;
}

double interpretAsDouble(u8* buffer) {
  u8 swapped[8]
      = { buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7] };

  double result;
  std::memcpy(&result, swapped, sizeof(result));
  return result;
}

Value interpretDataType(eRequestDataType dataType, u8* buffer, u64 count,
                        MemoryPoolAllocator<CrtAllocator>& allocator) {
  Value contents(kArrayType);
  contents.Reserve(count, allocator);

  u64 stride = sizeFromType(dataType);
  for (u64 i = 0; i < count; i++) {
    u8* ptr = buffer + i * stride;
    switch (dataType) {
      case eRequestDataType_u8:
      case eRequestDataType_u16:
      case eRequestDataType_u32:
      case eRequestDataType_u64:
        contents.PushBack(Value(convertByteArrayToHex(ptr, stride).c_str(), allocator).Move(),
                          allocator);
        break;

      case eRequestDataType_f64:
        contents.PushBack(Value(interpretAsDouble(ptr)).Move(), allocator);
        break;

      case eRequestDataType_f32:
        contents.PushBack(Value(interpretAsFloat(ptr)).Move(), allocator);
        break;

      case eRequestDataType_i64:
        contents.PushBack(Value(*(s64*)ptr).Move(), allocator);
        break;

      case eRequestDataType_i16:
        contents.PushBack(Value(*(s16*)ptr).Move(), allocator);
        break;

      case eRequestDataType_i32:
        contents.PushBack(Value(*(s32*)ptr).Move(), allocator);
        break;

      default:
        break;
    }
  }

  return contents;
}
