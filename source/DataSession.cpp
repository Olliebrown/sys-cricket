#include "DataSession.h"

#include "DataBlock.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <sstream>

using namespace rapidjson;

DataSession::DataSession(sockaddr_in clientAddr, const ConfigMessage& streamMessage,
                         int sockStream) :
    StreamSession(clientAddr, sockStream) {
  // Create the data block
  this->dataBlock = new DataBlock(getClientKey(clientAddr, streamMessage.nickname), streamMessage);

  // Set the data type and count and allocate the buffer
  this->dataType = streamMessage.dataType;
  this->dataCount = streamMessage.dataCount;
  this->nickname = streamMessage.nickname;
  this->buffer = malloc(sizeFromType(this->dataType) * this->dataCount);
}

DataSession::~DataSession() {
  // Stop stream first
  stopStream();

  // Delete the data block
  delete dataBlock;

  // Free the local buffer
  free(this->buffer);
}

bool DataSession::streamSendStatus() {
  // Get the status
  char* metadataJson;
  Result result = dataBlock->GetStatus(metadataJson);
  bool success = false;
  if (R_SUCCEEDED(result)) {
    // Send the status
    success = streamSendData(metadataJson);
    free(metadataJson);
  } else {

    Document contents(kObjectType);
    auto a = contents.GetAllocator();
    contents.AddMember("messageType", Value("status", a).Move(), a);
    contents.AddMember("error", Value("Failed to get status", a).Move(), a);
    contents.AddMember("module", Value(R_MODULE(result)).Move(), a);
    contents.AddMember("description", Value(R_DESCRIPTION(result)), a);
    success = streamSendData(contents);
  }

  return success;
}

bool DataSession::readAndSendData() {
  // Prepare the JSON document
  Document contents(kObjectType);
  contents.AddMember("messageType", "data", contents.GetAllocator());
  if (!this->nickname.empty()) {
    contents.AddMember("nickname", Value(this->nickname.c_str(), contents.GetAllocator()).Move(),
                       contents.GetAllocator());
  }

  // Read the data
  Result result = dataBlock->ReadMemory(this->buffer);
  if (R_SUCCEEDED(result)) {
    // Turn data into JSON message
    Value rawData = interpretDataType(dataType, (u8*)buffer, dataCount, contents.GetAllocator());
    contents.AddMember("data", rawData.Move(), contents.GetAllocator());
  } else {
    fprintf(stderr, "Server: failed to read data\n");
    contents.AddMember("error", "Failed to read data", contents.GetAllocator());
    contents.AddMember("module", R_MODULE(result), contents.GetAllocator());
    contents.AddMember("description", R_DESCRIPTION(result), contents.GetAllocator());
  }

  return streamSendData(contents);
}

bool DataSession::pokeData(void* data) {
  Result result = dataBlock->WriteMemory(this->buffer);
  return R_SUCCEEDED(result);
}
