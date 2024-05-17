#include <switch.h>

#include <arpa/inet.h>
#include <map>
#include <string>

class StreamSession {
 public:
  StreamSession(struct sockaddr_in clientAddr);
  ~StreamSession();

  Waiter startStream(uint64_t interval);
  void stopStream();
  bool streamSendData(std::string& message) const;

  static uint64_t getClientKey(sockaddr_in sockAddr);

 protected:
  int sockStream;
  struct sockaddr_in clientAddr;
  std::map<uint64_t, int> streamSockets;
  UTimer connectTimer;

  bool initSocket();
  bool remoteConnect();
};
