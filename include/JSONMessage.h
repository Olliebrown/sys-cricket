#include <cstdint>
#include <types.h>

enum eConnectType { E_TYPE_CONNECT = 0, E_TYPE_DISCONNECT = 1, E_TYPE_UNKNOWN = 2 };

struct ConnectMessage {
  eConnectType type;
  uint16_t port;
  u64* offsets;
  size_t offsetCount;
  size_t blockSize;
};

ConnectMessage makeConnectMessage(const char* message);
void deallocConnectMessage(ConnectMessage message);
