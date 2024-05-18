#include <switch.h>

#include <arpa/inet.h>
#include <map>
#include <string>

#include <rapidjson/document.h>

class StreamSession {
 public:
  StreamSession(struct sockaddr_in clientAddr, int sockStream = 0);
  virtual ~StreamSession();

  int getUseCount() const { return useCount; }
  int useSocket() {
    useCount++;
    return sockStream;
  }
  void freeSocket() {
    if (useCount > 0) {
      useCount--;
    }
  }

  virtual Waiter startStream(uint64_t interval);
  virtual void stopStream();

  virtual bool streamSendData(const rapidjson::Document& message);
  virtual bool streamSendData(const char* message);

  virtual std::string getMyClientKey() const { return getClientKey(clientAddr, ""); }
  static std::string getClientKey(sockaddr_in sockAddr, std::string nickname);

 protected:
  int sockStream, useCount;
  struct sockaddr_in clientAddr;
  UTimer intervalTimer;
  bool socketOwner;

  bool initSocket();
  bool remoteConnect();
};
