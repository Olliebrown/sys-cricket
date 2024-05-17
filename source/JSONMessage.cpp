#include "JSONMessage.h"
#include "JSONMessageSchema.h"

#include <rapidjson/document.h>
#include <rapidjson/schema.h>
using namespace rapidjson;

SchemaValidator* getSchemaValidator() {
  static SchemaDocument* configSchemaDoc = nullptr;
  static SchemaValidator* configSchemaValidator = nullptr;

  if (configSchemaValidator != nullptr) {
    return configSchemaValidator;
  }

  Document configSchemaDOM;
  configSchemaDOM.Parse(configMessageSchema);
  configSchemaDoc = new SchemaDocument(configSchemaDOM);
  configSchemaValidator = new SchemaValidator(*configSchemaDoc);
  return configSchemaValidator;
}

ConfigMessage::ConfigMessage(const char* message) {
  this->type = E_TYPE_UNKNOWN;
  this->port = 0;
  this->offsets = nullptr;
  this->offsetCount = 0;
  this->blockSize = 0;
  this->nsInterval = 3333333;  // 1/30th of a second

  // Attempt to parse the JSON message
  Document DOM;
  if (DOM.Parse(message).HasParseError()) {
    fprintf(stderr, "Error parsing JSON message.\n");
    return;
  }

  // Validate the message against the schema
  SchemaValidator* validator = getSchemaValidator();
  if (!DOM.Accept(*validator)) {
    StringBuffer sb;
    validator->GetInvalidSchemaPointer().StringifyUriFragment(sb);
    fprintf(stderr, "Invalid schema: %s\n", sb.GetString());
    fprintf(stderr, "Invalid keyword: %s\n", validator->GetInvalidSchemaKeyword());
    sb.Clear();

    return;
  }

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

      if (DOM.HasMember("nsInterval")) {
        this->nsInterval = DOM["nsInterval"].GetUint64();
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
