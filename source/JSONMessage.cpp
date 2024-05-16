#include "JSONMessage.h"

#include <rapidjson/document.h>
using namespace rapidjson;

ConnectMessage makeConnectMessage(const char* message) {
  Document DOM;
  DOM.Parse(message);

  ConnectMessage connectMessage = { E_TYPE_UNKNOWN, 0, nullptr, 0, 0 };
  // All messages must have a port and type
  if (DOM.HasMember("port") && DOM.HasMember("type")) {
    if (DOM["type"].GetString() == "connect") {
      connectMessage.type = E_TYPE_CONNECT;
    } else if (DOM["type"].GetString() == "disconnect") {
      connectMessage.type = E_TYPE_DISCONNECT;
    }

    connectMessage.port = DOM["port"].GetUint();

    // If this is a new connection message, parse the offsets and block size
    if (connectMessage.type == E_TYPE_CONNECT) {
      if (DOM.HasMember("offsets")) {
        connectMessage.offsetCount = DOM["offsets"].Size();
        connectMessage.offsets = new u64[connectMessage.offsetCount];
        for (int i = 0; i < connectMessage.offsetCount; i++) {
          connectMessage.offsets[i] = DOM["offsets"][i].GetUint64();
        }
      }

      if (DOM.HasMember("blockSize")) {
        connectMessage.blockSize = DOM["blockSize"].GetUint();
      }
    }
  }

  return connectMessage;
}

void deallocConnectMessage(ConnectMessage message) {
  if (message.offsets != nullptr) {
    delete[] message.offsets;
    message.offsets = nullptr;
    message.offsetCount = 0;
  }
}
