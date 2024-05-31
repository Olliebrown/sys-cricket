#pragma once

#include "JSONMessage.h"
#include "StreamSession.h"

class DataBlock;

class DataSession : public StreamSession {
 public:
  DataSession(sockaddr_in clientAddr, const ConfigMessage& streamMessage, int sockStream = 0);
  virtual ~DataSession();

  bool streamSendStatus();
  bool readAndSendData();
  bool pokeData(void* data);

  std::string getMyClientKey() const override { return getClientKey(clientAddr, nickname); }

 protected:
  DataBlock* dataBlock;
  void* buffer;
  eRequestDataType dataType;
  uint64_t dataCount;
  std::string nickname;
};
