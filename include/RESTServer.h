#include "ThreaddedServer.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <map>
#include <string>
#include <vector>

class RESTServer : public ThreadedServer {
 public:
  RESTServer();
  virtual ~RESTServer() { }

 protected:
  // Sockets and addresses
  int sockConn, sockStream;
  struct sockaddr_in client, server;
  std::map<u64, int> streamSockets;

  // Internal timers and events
  UTimer connectTimer;
  UEvent exitEvent;

  // Timers and waitable objects for all requested data blocks
  std::vector<UTimer> timers;
  std::vector<Waiter> waiters;

  // Override thread methods
  virtual bool threadInit();
  virtual void threadMain();
  virtual void threadExit();

  // Override server control function
  virtual void serverMain();

  // Communication functions
  bool initSocket(int& sockOut, sockaddr_in addrConfig, const std::string& description);
  bool remoteConnect(int& sockOut, sockaddr_in addrConfig);

  bool connectionReceive();
  bool streamSendData();
};
