#include "RESTServer.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <sys/errno.h>
#include <poll.h>

#include <string>

// Port to listen for connection messages (UDP)
#define LISTEN_PORT 3000

enum eEventID {
  EVENT_ID_UPDATE = 0,
  EVENT_ID_EXIT
};

RESTServer::RESTServer() : ThreadedServer(false) {
  // Set up the connection listening address
  server.sin_family = AF_INET;
  server.sin_port = htons(LISTEN_PORT);
  server.sin_addr.s_addr = INADDR_ANY;

  // Clear the client address
  memset(&client, 0, sizeof(client));

  // Clear the socket handle and port
  sockConn = sockStream = 0;
}

bool RESTServer::threadInit() {
  // Initialize timer and event handles
  utimerCreate(&updateTimer, 10000000, TimerType_Repeating); // 1/100s
  ueventCreate(&exitEvent, false);

  // Initialize the connection socket
  if(!initSocket(sockConn, server, "connection")) {
    close(sockConn);
    sockConn = 0;
    return false;
  }

  return true;
}

void RESTServer::threadMain() {
  Result rc;
  eEventID event;

  // Loop until exit event
  bool exitRequested = false;
  while (!exitRequested) {
    // Wait for either timer or an exit event
    rc = waitMulti((int*)&event, -1, waiterForUTimer(&updateTimer), waiterForUEvent(&exitEvent));
    if (R_SUCCEEDED(rc)) {
      switch (event) {
        case EVENT_ID_UPDATE:
          if (sockConn > 0) { connectionReceive(); }
          if (sockStream > 0) { streamSendData(); }
          break;

        case EVENT_ID_EXIT:
          exitRequested = true;
          break;
      }
    }
  }
}

// Send the thread the exit event
void RESTServer::threadExit() {
  ueventSignal(&exitEvent);
  if (sockConn > 0) { close(sockConn); }
  if (sockStream > 0) { close(sockStream); }
  sockConn = sockStream = 0;
}

// Server control function
void RESTServer::serverMain() {
  utimerStart(&updateTimer);
}

bool RESTServer::initSocket(int &sockOut, sockaddr_in addrConfig, const std::string &description) {
  // Create a UDP socket
  sockOut = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockOut < 0) {
    perror("Server: failed to create %s socket", description.c_str());
    return false;
  }

  // Set to non-blocking mode
  int flags = fcntl(sockOut, F_GETFL, 0);
  if (flags == -1) {
    perror("Server: can't set non-blocking / failed to read %s socket flags.\n", description.c_str());
  } else if (fcntl(sockOut, F_SETFL, flags | O_NONBLOCK) != 0) {
    perror("Server: failed to set %s socket to non-blocking io.\n", description.c_str());
  }

  // Is this a local or remote connection?
  if (addrConfig.sin_addr.s_addr == INADDR_ANY) {
    // Bind to a local port
    if (bind(sockOut, (struct sockaddr*)&addrConfig, sizeof(addrConfig)) < 0) {
      perror("Server: failed to bind %s socket", description.c_str());
      return false;
    }

    // Output the port number
    printf("The %s socket is listening on port %d\n", description.c_str(), ntohs(addrConfig.sin_port));
  } else {
    if (!remoteConnect(sockOut, addrConfig)) {
      perror("Server: failed remote connection to %s:%d", inet_ntoa(addrConfig.sin_addr), ntohs(addrConfig.sin_port));
      return false;
    } else {
      printf("The %s socket will send to %s:%d\n", description.c_str(), inet_ntoa(addrConfig.sin_addr), ntohs(addrConfig.sin_port));
    }
  }

  // Return success
  return true;
}

bool RESTServer::connectionReceive() {
  static char buf[4096];
  socklen_t client_address_size = sizeof(client);

  ssize_t recvLen = 0;
  if((recvLen = recvfrom(sockConn, buf, sizeof(buf) - 1, 0, (struct sockaddr *) &client, &client_address_size)) < 0)  {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      perror("Server: error listening for message (%l)", recvLen);
      return false;
    }
  }

  // Process the received message
  if (recvLen > 0) {
    printf("Connection message from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    buf[recvLen] = '\0'; // Null-terminate the string
    printf("\t> %s\n", buf);
    client.sin_port = htons(strtoul(buf, NULL, 10));

    // Setup the streaming socket for this client
    if(!initSocket(sockStream, client, "streaming")) {
      close(sockStream);
      sockStream = 0;
      return false;
    }
  }

  return true;
}

bool RESTServer::remoteConnect(int &sockOut, sockaddr_in addrConfig) {
  // Connect ot the host
  int ret = connect(sockOut, (struct sockaddr *)&addrConfig, sizeof(addrConfig));
  if (ret != 0 && errno != EINPROGRESS) {
    perror("Server: remote connection failed (%d).\n", errno);
    return false;
  }

  // Possibly wait for connection to complete
  if (ret != 0) {
    // Setup the socket polling struct
    struct pollfd pfd;
    pfd.fd      = sockOut;
    pfd.events  = POLLOUT;
    pfd.revents = 0;

    // Wait up to 1s to connect
    int n = poll(&pfd, 1, 1000);
    if (n < 0) {
      perror("Server: remote connection poll failed.\n");
      return false;
    }

    // Timed out
    if (n == 0 || !(pfd.revents & POLLOUT)) {
      errno = ETIMEDOUT;
      perror("Server: remote connection timed out.\n");
      return false;
    }
  }

  // Return success
  return true;
}

bool RESTServer::streamSendData() {
  // Send the data
  std::string message = "Hello from the server!";
  ssize_t sentLen = send(sockStream, message.c_str(), message.length(), 0);
  if (sentLen < 0) {
    perror("Server: failed to send data to %s:%d.\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    perror("Server: error code %d.\n", errno);
    return false;
  }

  // Return success
  return true;
}
