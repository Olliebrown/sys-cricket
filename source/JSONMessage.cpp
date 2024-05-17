#include "JSONMessage.h"

#include <rapidjson/document.h>
using namespace rapidjson;

ConfigMessage::ConfigMessage(const char* message) {
  Document DOM;
  DOM.Parse(message);

  this->type = E_TYPE_UNKNOWN;
  this->port = 0;
  this->offsets = nullptr;
  this->offsetCount = 0;
  this->blockSize = 0;

  // All messages must have a port and type
  if (DOM.HasMember("port") && DOM.HasMember("type")) {
    if (strcmp(DOM["type"].GetString(), "connect") == 0) {
      this->type = E_TYPE_CONNECT;
    } else if (strcmp(DOM["type"].GetString(), "disconnect") == 0) {
      this->type = E_TYPE_DISCONNECT;
    }

    this->port = DOM["port"].GetUint();

    // If this is a new connection message, parse the offsets and block size
    if (this->type == E_TYPE_CONNECT) {
      if (DOM.HasMember("offsets")) {
        this->offsetCount = DOM["offsets"].Size();
        this->offsets = new uint64_t[this->offsetCount];
        for (size_t i = 0; i < this->offsetCount; i++) {
          this->offsets[i] = DOM["offsets"][i].GetUint64();
        }
      }

      if (DOM.HasMember("blockSize")) {
        this->blockSize = DOM["blockSize"].GetUint();
      }
    }
  }
}

ConfigMessage::~ConfigMessage() {
  if (this->offsets != nullptr) {
    delete[] this->offsets;
    this->offsets = nullptr;
    this->offsetCount = 0;
  }
}
