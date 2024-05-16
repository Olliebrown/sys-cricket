#include <switch.h>

#include <arpa/inet.h>
#include <map>
#include <string>
#include <types.h>

class StreamSession {
 public:
  StreamSession(struct sockaddr_in clientAddr);
  ~StreamSession();

  Waiter startStream(u64 interval);
  void stopStream();
  bool streamSendData(std::string& message) const;

 protected:
  int sockStream;
  struct sockaddr_in clientAddr;
  std::map<u64, int> streamSockets;
  UTimer connectTimer;

  bool initSocket();
  bool remoteConnect();
};
