#include "ThreaddedServer.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <map>
#include <string>
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
  std::map<u64, DataSession*> streams;

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
};
