#include "StreamSession.h"

#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <switch.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
using namespace rapidjson;

StreamSession::StreamSession(sockaddr_in clientAddr, int sockStream) {
  this->clientAddr = clientAddr;
  this->sockStream = sockStream;
  this->socketOwner = false;

  if (this->clientAddr.sin_addr.s_addr != 0 && this->clientAddr.sin_port != 0) {
    if (sockStream <= 0) {
      initSocket();
      this->socketOwner = true;
    }
  }
}

StreamSession::~StreamSession() {
  stopStream();
  if (socketOwner && sockStream > 0) {
    close(sockStream);
    sockStream = 0;
  }
}

std::string StreamSession::getClientKey(sockaddr_in sockAddr, std::string nickname) {
  return std::to_string((uint64_t)sockAddr.sin_addr.s_addr << 16 | sockAddr.sin_port) + nickname;
}

Waiter StreamSession::startStream(uint64_t interval) {
  // Start the timer
  utimerCreate(&intervalTimer, interval, TimerType_Repeating);
  utimerStart(&intervalTimer);

  // Return the waiter
  return waiterForUTimer(&intervalTimer);
}

void StreamSession::stopStream() {
  utimerStop(&intervalTimer);
}

bool StreamSession::streamSendData(const Document& message) {
  StringBuffer strBuffer;
  Writer<StringBuffer> writer(strBuffer);
  message.Accept(writer);
  return streamSendData(strBuffer.GetString());
}

bool StreamSession::streamSendData(const char* message) {
  // Send the data
  ssize_t sentLen = send(sockStream, message, strlen(message), 0);
  if (sentLen < 0) {
    fprintf(stderr, "Server: failed to send data to %s:%d.\n", inet_ntoa(clientAddr.sin_addr),
            ntohs(clientAddr.sin_port));
    fprintf(stderr, "Server: error code %d.\n", errno);
    return false;
  }

  // Return success
  return true;
}

bool StreamSession::initSocket() {
  // Create a UDP socket
  sockStream = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockStream < 0) {
    fprintf(stderr, "Stream Session: failed to create stream socket");
    return false;
  }

  // Set to non-blocking mode
  int flags = fcntl(sockStream, F_GETFL, 0);
  if (flags == -1) {
    fprintf(stderr,
            "Stream Session: can't set non-blocking / failed to read stream socket flags.\n");
  } else if (fcntl(sockStream, F_SETFL, flags | O_NONBLOCK) != 0) {
    fprintf(stderr, "Stream Session: failed to set stream socket to non-blocking io.\n");
  }

  if (!remoteConnect()) {
    fprintf(stderr, "Stream Session: failed remote connection to %s:%d",
            inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    return false;
  } else {
    printf("Stream Session: The stream socket will send to %s:%d\n", inet_ntoa(clientAddr.sin_addr),
           ntohs(clientAddr.sin_port));
  }

  // Return success
  return true;
}

bool StreamSession::remoteConnect() {
  // Connect ot the host
  int ret = connect(sockStream, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
  if (ret != 0 && errno != EINPROGRESS) {
    fprintf(stderr, "Stream Session: remote connection failed (%d).\n", errno);
    return false;
  }

  // Possibly wait for connection to complete
  if (ret != 0) {
    // Setup the socket polling struct
    struct pollfd pfd;
    pfd.fd = sockStream;
    pfd.events = POLLOUT;
    pfd.revents = 0;

    // Wait up to 1s to connect
    int n = poll(&pfd, 1, 1000);
    if (n < 0) {
      fprintf(stderr, "Stream Session: remote connection poll failed.\n");
      return false;
    }

    // Timed out
    if (n == 0 || !(pfd.revents & POLLOUT)) {
      errno = ETIMEDOUT;
      fprintf(stderr, "Stream Session: remote connection timed out.\n");
      return false;
    }
  }

  // Return success
  return true;
}
