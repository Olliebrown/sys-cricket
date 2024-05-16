#include "ThreaddedServer.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <map>
#include <string>
#include <vector>

#include "StreamSession.h"

class RESTServer : public ThreadedServer {
 public:
  RESTServer();
  virtual ~RESTServer() { }

 protected:
  // Sockets and addresses
  int sockConn;
  struct sockaddr_in server;
  std::map<u64, StreamSession*> streams;

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
