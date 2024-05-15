#include "ThreaddedServer.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <string>

class RESTServer : public ThreadedServer {
public:
  RESTServer();
  virtual ~RESTServer() {}

protected:
  // Sockets and addresses
  int sockConn, sockStream;
  struct sockaddr_in client, server;

  // Internal timers and events
  UTimer updateTimer;
  UEvent exitEvent;

  // Override thread methods
  virtual bool threadInit();
  virtual void threadMain();
  virtual void threadExit();

  // Override server control function
  virtual void serverMain();

  // Communication functions
  bool initSocket(int &sockOut, sockaddr_in addrConfig, const std::string &description);
  bool remoteConnect(int &sockOut, sockaddr_in addrConfig);

  bool connectionReceive();
  bool streamSendData();
};
