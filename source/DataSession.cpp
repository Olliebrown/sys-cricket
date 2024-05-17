#include "DataSession.h"

#include "DataBlock.h"

DataSession::DataSession(sockaddr_in clientAddr, const ConfigMessage& streamMessage) :
    StreamSession(clientAddr) {
  // Create the data block
  dataBlock = new DataBlock(getClientKey(clientAddr), streamMessage);
}

DataSession::~DataSession() {
  // Delete the data block
  delete dataBlock;
}
