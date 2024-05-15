#include "utils.h"

#include <iomanip>
#include <vector>
#include <cstring>

std::string convertByteArrayToHex(u8 *bytes, size_t size) {
  std::stringstream stream;

  stream << std::setfill('0') << std::hex;

  for (u64 i = 0; i < size; i++) {
    stream << std::setw(2) << static_cast<int>(bytes[i]);
  }

  return stream.str();
}

std::string convertNumToHexString(u64 num, int width, bool withPrefix) {
  std::stringstream hex;
  if (withPrefix) { hex << "0x"; }
  hex << std::setfill('0') << std::setw(width) << std::hex << num;
  return hex.str();
}

int sizeFromType(eRequestDataType dataType) {
  switch(dataType) {
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

float interpretAsFloat(u8* buffer)
{
  u8 swapped[4] = { buffer[0], buffer[1], buffer[2], buffer[3] };

  float result;
  std::memcpy(&result, swapped, 4);
  return result;
}

double interpretAsDouble(u8* buffer)
{
  u8 swapped[8] = {buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]};

  double result;
  std::memcpy(&result, swapped, sizeof(result));
  return result;
}

bool getParams(const char* body, std::string &offsetStr, eRequestDataType &dataType, u64 &count) {
  // Read URL parameters
  // if (req.has_param("offset")) {
  //   offsetStr = req.get_param_value("offset");
  // }

  // if (req.has_param("count")) {
  //   std::string countStr = req.get_param_value("count");
  //   try {
  //     count = stoull(countStr, 0, 10);
  //   } catch (const std::invalid_argument &ia) {
  //     res.set_content("Invalid count", "text/plain");
  //     res.status = 400;
  //     return false;
  //   }
  // }
  
  // if (req.has_param("type")) {
  //   std::string dataTypeStr = req.get_param_value("type");
  //   if (dataTypeStr == "f64") {
  //     dataType = eRequestDataType_f64;
  //   } else if (dataTypeStr == "f32") {
  //     dataType = eRequestDataType_f32;
  //   } else if (dataTypeStr == "i64") {
  //     dataType = eRequestDataType_i64;
  //   } else if (dataTypeStr == "i32") {
  //     dataType = eRequestDataType_i32;
  //   } else if (dataTypeStr == "i16") {
  //     dataType = eRequestDataType_i16;
  //   } else if (dataTypeStr == "u64") {
  //     dataType = eRequestDataType_u64;
  //   } else if (dataTypeStr == "u32") {
  //     dataType = eRequestDataType_u32;
  //   } else if (dataTypeStr == "u16") {
  //     dataType = eRequestDataType_u16;
  //   } else if (dataTypeStr == "u8") {
  //     dataType = eRequestDataType_u8;
  //   } else {
  //     dataType = eRequestDataType_Invalid;
  //     res.set_content("Invalid data type", "text/plain");
  //     res.status = 400;
  //     return false;
  //   }
  // }

  return true;
}

std::vector<std::string> interpretDataType(eRequestDataType dataType, u8 *buffer, u64 count) {
  std::vector<std::string> contents;

  u64 stride = sizeFromType(dataType);
  for(u64 i=0; i<count; i++) {
    u8 *ptr = buffer + i * stride;
    switch(dataType) {
      case eRequestDataType_u8:
      case eRequestDataType_u16:
      case eRequestDataType_u32:
      case eRequestDataType_u64:
        contents.push_back(convertByteArrayToHex(ptr, stride));
        break;
      
      case eRequestDataType_f64:
        contents.push_back(std::to_string(interpretAsDouble(ptr)));
        break;

      case eRequestDataType_f32:
        contents.push_back(std::to_string(interpretAsFloat(ptr)));
        break;

      case eRequestDataType_i64:
        contents.push_back(std::to_string(*(s64*)ptr));
        break;

      case eRequestDataType_i16:
        contents.push_back(std::to_string(*(s16*)ptr));
        break;

      case eRequestDataType_i32:
        contents.push_back(std::to_string(*(s32*)ptr));
        break;

      default:
        break;
    }
  }

  return contents;
}
