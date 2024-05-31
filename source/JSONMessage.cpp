#include "JSONMessage.h"
#include "JSONMessageSchema.h"

#include <rapidjson/document.h>
#include <rapidjson/schema.h>
using namespace rapidjson;

eConfigType decodeMessageType(const char* typeStr);
eRequestDataType decodeDataType(const char* typeStr);
void* readDataArray(const Document& DOM, const eRequestDataType& dataType, const size_t& dataCount);

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
  this->dataArray = nullptr;
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
  if (DOM.HasMember("port") && DOM.HasMember("messageType") && DOM["messageType"].IsString()
      && DOM["port"].IsUint()) {
    this->messageType = decodeMessageType(DOM["messageType"].GetString());
    this->port = DOM["port"].GetUint();

    // Might have nickname
    if (DOM.HasMember("nickname") && DOM["nickname"].IsString()) {
      this->nickname = std::string(DOM["nickname"].GetString(), DOM["nickname"].GetStringLength());
    }

    // If this is a new or poke connection message, parse the offsets and block size
    if (this->messageType == eConfigType_StartData || this->messageType == eConfigType_PokeData) {
      if (DOM.HasMember("offsets") && DOM["offsets"].IsArray()) {
        this->offsetCount = DOM["offsets"].Size();
        this->offsets = new uint64_t[this->offsetCount];
        for (size_t i = 0; i < this->offsetCount; i++) {
          if (DOM["offsets"][i].IsUint64()) {
            this->offsets[i] = DOM["offsets"][i].GetUint64();
          }
        }
      }

      if (DOM.HasMember("dataType") && DOM["dataType"].IsString()) {
        this->dataType = decodeDataType(DOM["dataType"].GetString());
      }

      if (DOM.HasMember("dataCount") && DOM["dataCount"].IsUint64()) {
        this->dataCount = DOM["dataCount"].GetUint64();
      }

      if (DOM.HasMember("dataArray") && DOM["dataArray"].IsArray()) {
        if (DOM["dataArray"].Size() != this->dataCount) {
          fprintf(stderr, "Data array length does not match the data count .\n");
        } else {
          this->dataArray = readDataArray(DOM, this->dataType, this->dataCount);
        }
      }

      if (DOM.HasMember("nsInterval") && DOM["nsInterval"].IsUint64()) {
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

  if (this->dataArray != nullptr) {
    switch (this->dataType) {
      case eRequestDataType_u8:
        delete[](uint8_t*) this->dataArray;
        break;
      case eRequestDataType_u16:
        delete[](uint16_t*) this->dataArray;
        break;
      case eRequestDataType_u32:
        delete[](uint32_t*) this->dataArray;
        break;
      case eRequestDataType_u64:
        delete[](uint64_t*) this->dataArray;
        break;
      case eRequestDataType_i16:
        delete[](int16_t*) this->dataArray;
        break;
      case eRequestDataType_i32:
        delete[](int32_t*) this->dataArray;
        break;
      case eRequestDataType_i64:
        delete[](int64_t*) this->dataArray;
        break;
      case eRequestDataType_f32:
        delete[](float*) this->dataArray;
        break;
      case eRequestDataType_f64:
        delete[](double*) this->dataArray;
        break;
      default:
        fprintf(stderr, "Invalid data type for freeing dataArray.\n");
        break;
    }
    this->dataArray = nullptr;
    this->dataCount = 0;
  }
}

eConfigType decodeMessageType(const char* typeStr) {
  if (strcmp(typeStr, "connect") == 0) {
    return eConfigType_Connect;
  } else if (strcmp(typeStr, "disconnect") == 0) {
    return eConfigType_Disconnect;
  } else if (strcmp(typeStr, "start") == 0) {
    return eConfigType_StartData;
  } else if (strcmp(typeStr, "poke") == 0) {
    return eConfigType_PokeData;
  } else if (strcmp(typeStr, "stop") == 0) {
    return eConfigType_StopData;
  } else if (strcmp(typeStr, "refresh") == 0) {
    return eConfigType_Refresh;
  } else if (strcmp(typeStr, "remoteLog") == 0) {
    return eConfigType_RemoteLog;
  }

  return eConfigType_Invalid;
}

eRequestDataType decodeDataType(const char* typeStr) {
  if (strcmp(typeStr, "u8") == 0) {
    return eRequestDataType_u8;
  } else if (strcmp(typeStr, "u16") == 0) {
    return eRequestDataType_u16;
  } else if (strcmp(typeStr, "u32") == 0) {
    return eRequestDataType_u32;
  } else if (strcmp(typeStr, "u64") == 0) {
    return eRequestDataType_u64;
  } else if (strcmp(typeStr, "i16") == 0) {
    return eRequestDataType_i16;
  } else if (strcmp(typeStr, "i32") == 0) {
    return eRequestDataType_i32;
  } else if (strcmp(typeStr, "i64") == 0) {
    return eRequestDataType_i64;
  } else if (strcmp(typeStr, "f32") == 0) {
    return eRequestDataType_f32;
  } else if (strcmp(typeStr, "f64") == 0) {
    return eRequestDataType_f64;
  } else {
    return eRequestDataType_Invalid;
  }
}

void* readDataArray(const Document& DOM, const eRequestDataType& dataType,
                    const size_t& dataCount) {
  void* dataArray = nullptr;
  switch (dataType) {
    case eRequestDataType_u8:
      dataArray = new uint8_t[dataCount];
      break;
    case eRequestDataType_u16:
      dataArray = new uint16_t[dataCount];
      break;
    case eRequestDataType_u32:
      dataArray = new uint32_t[dataCount];
      break;
    case eRequestDataType_u64:
      dataArray = new uint64_t[dataCount];
      break;
    case eRequestDataType_i16:
      dataArray = new int16_t[dataCount];
      break;
    case eRequestDataType_i32:
      dataArray = new int32_t[dataCount];
      break;
    case eRequestDataType_i64:
      dataArray = new int64_t[dataCount];
      break;
    case eRequestDataType_f32:
      dataArray = new float[dataCount];
      break;
    case eRequestDataType_f64:
      dataArray = new double[dataCount];
      break;
    default:
      fprintf(stderr, "Invalid data type for readDataArray.\n");
      return nullptr;
  }

  for (size_t i = 0; i < dataCount; i++) {
    switch (dataType) {
      case eRequestDataType_u8:
        if (DOM["dataArray"][i].IsUint()) {
          ((uint8_t*)dataArray)[i] = (uint8_t)DOM["dataArray"][i].GetUint();
        }
        break;
      case eRequestDataType_u16:
        if (DOM["dataArray"][i].IsUint()) {
          ((uint16_t*)dataArray)[i] = (uint16_t)DOM["dataArray"][i].GetUint();
        }
        break;

      case eRequestDataType_u32:
        if (DOM["dataArray"][i].IsUint()) {
          ((uint32_t*)dataArray)[i] = DOM["dataArray"][i].GetUint();
        }
        break;

      case eRequestDataType_u64:
        if (DOM["dataArray"][i].IsUint64()) {
          ((uint64_t*)dataArray)[i] = DOM["dataArray"][i].GetUint64();
        }
        break;

      case eRequestDataType_i16:
        if (DOM["dataArray"][i].IsInt()) {
          ((int16_t*)dataArray)[i] = (int16_t)DOM["dataArray"][i].GetInt();
        }
        break;

      case eRequestDataType_i32:
        if (DOM["dataArray"][i].IsInt()) {
          ((int32_t*)dataArray)[i] = DOM["dataArray"][i].GetInt();
        }
        break;

      case eRequestDataType_i64:
        if (DOM["dataArray"][i].IsInt64()) {
          ((int64_t*)dataArray)[i] = DOM["dataArray"][i].GetInt64();
        }
        break;

      case eRequestDataType_f32:
        if (DOM["dataArray"][i].IsFloat()) {
          ((float*)dataArray)[i] = DOM["dataArray"][i].GetFloat();
        }
        break;

      case eRequestDataType_f64:
        if (DOM["dataArray"][i].IsDouble()) {
          ((double*)dataArray)[i] = DOM["dataArray"][i].GetDouble();
        }
        break;

      default:
        fprintf(stderr, "Invalid data type for readDataArray.\n");
        return nullptr;
    }
  }

  return dataArray;
}
