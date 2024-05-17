#pragma once

#include "JSONMessage.h"
#include "StreamSession.h"

class DataBlock;

class DataSession : public StreamSession {
 public:
  DataSession(sockaddr_in clientAddr, const ConfigMessage& streamMessage);
  ~DataSession();

 protected:
  DataBlock* dataBlock;
};
