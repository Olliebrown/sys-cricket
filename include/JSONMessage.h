#pragma once

#include <cstdint>

enum eConfigType { E_TYPE_CONNECT = 0, E_TYPE_DISCONNECT = 1, E_TYPE_UNKNOWN = 2 };

struct ConfigMessage {
  ConfigMessage(const char* message);
  ~ConfigMessage();

  eConfigType type;
  uint16_t port;
  uint64_t* offsets;
  size_t offsetCount;
  size_t blockSize;
};
