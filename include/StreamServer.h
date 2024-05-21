#pragma once

#include "ThreaddedServer.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "DataSession.h"

// 6 seconds in nanoseconds
#define TIMEOUT_INTERVAL 6E9

// Put a cap on the number of active streams (for DOS protection)
#define MAX_ACTIVE_STREAMS 50

class StreamServer : public ThreadedServer {
 public:
  StreamServer();
  virtual ~StreamServer() { }

 protected:
  // Sockets and addresses
  int sockConn, socketNXLink;
  struct sockaddr_in server;
  std::unordered_map<std::string, DataSession*> streams;
  std::vector<std::string> streamKeys;

  // Internal timers and events
  UTimer connectTimer;
  UEvent exitEvent;

  // Waitable objects for all requested data blocks
  std::vector<Waiter> waiters;

  // Override thread methods
  virtual bool threadInit();
  virtual void threadMain();
  virtual void threadExit();

  // Override server control function
  virtual void serverMain();

  // Communication functions
  bool initConnectionSocket();
  bool connectionReceive();
  bool handleConnectionMessage(struct sockaddr_in& client, const ConfigMessage& message,
                               const std::string& parentKey, const std::string& clientKey);

  bool startParentSession(struct sockaddr_in& client, const ConfigMessage& message,
                          const std::string& parentKey);
  bool releaseParentSession(const std::string& parentKey, bool killChildren = true);

  bool startDataSession(struct sockaddr_in& client, const ConfigMessage& message,
                        const std::string& parentKey, const std::string& clientKey);
  bool releaseDataSession(const std::string& parentKey, const std::string& clientKey);
};
