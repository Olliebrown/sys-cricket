#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "utils.h"

enum eConfigType {
  eConfigType_Connect = 0,
  eConfigType_Disconnect = 1,
  eConfigType_StartData = 2,
  eConfigType_StopData = 3,
  eConfigType_Invalid = 4
};

struct ConfigMessage {
  ConfigMessage(const char* message);
  ~ConfigMessage();

  eConfigType messageType;
  uint16_t port;
  std::string nickname;
  uint64_t* offsets;
  size_t offsetCount;
  eRequestDataType dataType;
  size_t dataCount;
  uint64_t nsInterval;
};
