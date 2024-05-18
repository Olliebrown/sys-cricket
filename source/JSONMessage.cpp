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

ConfigMessage::ConfigMessage(const char* message) : nickname() {
  this->messageType = eConfigType_Invalid;
  this->port = 0;
  this->offsets = nullptr;
  this->offsetCount = 0;
  this->dataType = eRequestDataType_Invalid;
  this->dataCount = 0;
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
      this->messageType = eConfigType_Connect;
    } else if (strcmp(DOM["type"].GetString(), "disconnect") == 0) {
      this->messageType = eConfigType_Disconnect;
    } else if (strcmp(DOM["type"].GetString(), "start") == 0) {
      this->messageType = eConfigType_StartData;
    } else if (strcmp(DOM["type"].GetString(), "stop") == 0) {
      this->messageType = eConfigType_StopData;
    }

    this->port = DOM["port"].GetUint();

    // Might have nickname
    if (DOM.HasMember("nickname")) {
      this->nickname = std::string(DOM["nickname"].GetString(), DOM["nickname"].GetStringLength());
    }

    // If this is a new connection message, parse the offsets and block size
    if (this->messageType == eConfigType_StartData) {
      if (DOM.HasMember("offsets")) {
        this->offsetCount = DOM["offsets"].Size();
        this->offsets = new uint64_t[this->offsetCount];
        for (size_t i = 0; i < this->offsetCount; i++) {
          this->offsets[i] = DOM["offsets"][i].GetUint64();
        }
      }

      if (DOM.HasMember("dataType")) {
        if (strcmp(DOM["dataType"].GetString(), "u8") == 0) {
          this->dataType = eRequestDataType_u8;
        } else if (strcmp(DOM["dataType"].GetString(), "u16") == 0) {
          this->dataType = eRequestDataType_u16;
        } else if (strcmp(DOM["dataType"].GetString(), "u32") == 0) {
          this->dataType = eRequestDataType_u32;
        } else if (strcmp(DOM["dataType"].GetString(), "u64") == 0) {
          this->dataType = eRequestDataType_u64;
        } else if (strcmp(DOM["dataType"].GetString(), "i16") == 0) {
          this->dataType = eRequestDataType_i16;
        } else if (strcmp(DOM["dataType"].GetString(), "i32") == 0) {
          this->dataType = eRequestDataType_i32;
        } else if (strcmp(DOM["dataType"].GetString(), "i64") == 0) {
          this->dataType = eRequestDataType_i64;
        } else if (strcmp(DOM["dataType"].GetString(), "f32") == 0) {
          this->dataType = eRequestDataType_f32;
        } else if (strcmp(DOM["dataType"].GetString(), "f64") == 0) {
          this->dataType = eRequestDataType_f64;
        }
      }

      if (DOM.HasMember("dataCount")) {
        this->dataCount = DOM["dataCount"].GetUint64();
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
