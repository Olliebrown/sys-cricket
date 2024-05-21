#pragma once

#include <switch.h>

#include <arpa/inet.h>
#include <map>
#include <string>

#include <rapidjson/document.h>

class StreamSession {
 public:
  StreamSession(struct sockaddr_in clientAddr, int sockStream = 0);
  virtual ~StreamSession();

  inline bool isParent() const { return socketOwner; }
  inline bool childOf(const StreamSession& maybeParent) const {
    return maybeParent.sockStream == sockStream;
  }

  inline int getUseCount() const { return useCount; }
  inline int useSocket() {
    useCount++;
    return sockStream;
  }
  inline void freeSocket() {
    if (useCount > 0) {
      useCount--;
    }
  }

  virtual Waiter startStream(uint64_t interval);
  virtual void stopStream();

  inline void setHeartbeat(bool observed) { heartbeat = observed; }
  inline bool checkForHeartbeat(bool clear = true) {
    bool observed = heartbeat;
    if (clear) {
      heartbeat = false;
    }
    return observed;
  }

  virtual bool streamSendData(const rapidjson::Document& message);
  virtual bool streamSendData(const char* message);

  virtual std::string getMyClientKey() const { return getClientKey(clientAddr, ""); }
  static std::string getClientKey(sockaddr_in sockAddr, std::string nickname);

 protected:
  int sockStream, useCount;
  struct sockaddr_in clientAddr;
  UTimer intervalTimer;
  bool socketOwner;
  bool heartbeat;

  bool initSocket();
  bool remoteConnect();
};
