#include "ThreaddedServer.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "DataSession.h"

class StreamServer : public ThreadedServer {
 public:
  StreamServer();
  virtual ~StreamServer() { }

 protected:
  // Sockets and addresses
  int sockConn;
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
  bool handleConnectionMessage(struct sockaddr_in client, const ConfigMessage& message,
                               std::string parentKey, std::string clientKey);
};
