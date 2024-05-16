#include "RESTServer.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <poll.h>
#include <sys/errno.h>

#include <string>

#include "JSONMessage.h"

// Port to listen for connection messages (UDP)
#ifndef LISTENING_SOCKET_PORT
#define LISTENING_SOCKET_PORT 3000
#endif

enum eEventID { EVENT_INDEX_UPDATE = 0, EVENT_INDEX_EXIT };

RESTServer::RESTServer() : ThreadedServer(false) {
  // Set up the connection listening address
  server.sin_family = AF_INET;
  server.sin_port = htons(LISTENING_SOCKET_PORT);
  server.sin_addr.s_addr = INADDR_ANY;

  // Clear the client address
  memset(&client, 0, sizeof(client));

  // Clear the socket handle and port
  sockConn = sockStream = 0;
}

bool RESTServer::threadInit() {
  // Initialize timer and event handles
  utimerCreate(&connectTimer, 100000000, TimerType_Repeating);  // 1/10s
  ueventCreate(&exitEvent, false);
  waiters.push_back(waiterForUTimer(&connectTimer));
  waiters.push_back(waiterForUEvent(&exitEvent));

  // Initialize the connection socket
  if (!initSocket(sockConn, server, "connection")) {
    close(sockConn);
    sockConn = 0;
    return false;
  }

  return true;
}

void RESTServer::threadMain() {
  Result rc;
  int index;

  // Loop until exit event
  bool exitRequested = false;
  while (!exitRequested) {
    // Wait for the next event
    rc = waitObjects(&index, waiters.data(), waiters.size(), -1);
    if (R_SUCCEEDED(rc)) {
      switch (index) {
        // Check for new connections
        case EVENT_INDEX_UPDATE:
          if (sockConn > 0) {
            connectionReceive();
          }
          break;

        // Was an exit requested?
        case EVENT_INDEX_EXIT:
          exitRequested = true;
          break;

        // Other timer events
        default:
          if (index >= 0 && index < waiters.size()) {
          } else {
            fprintf(stdout, "Unknown event index %d\n", index);
          }
          break;
      }
    }
  }
}

// Send the thread the exit event
void RESTServer::threadExit() {
  // Signal the exit event
  ueventSignal(&exitEvent);

  // Stop all timers
  utimerStop(&connectTimer);
  for (auto& timer : timers) {
    utimerStop(&timer);
  }
  timers.clear();

  // Close and clear all sockets
  if (sockConn > 0) {
    close(sockConn);
  }
  if (sockStream > 0) {
    close(sockStream);
  }
  sockConn = sockStream = 0;
  for (auto& sock : streamSockets) {
    close(sock.second);
  }
  streamSockets.clear();
}

// Server control function
void RESTServer::serverMain() {
  utimerStart(&connectTimer);
}

bool RESTServer::initSocket(int& sockOut, sockaddr_in addrConfig, const std::string& description) {
  // Create a UDP socket
  sockOut = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockOut < 0) {
    perror("Server: failed to create %s socket", description.c_str());
    return false;
  }

  // Set to non-blocking mode
  int flags = fcntl(sockOut, F_GETFL, 0);
  if (flags == -1) {
    perror("Server: can't set non-blocking / failed to read %s socket flags.\n",
           description.c_str());
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
    printf("The %s socket is listening on port %d\n", description.c_str(),
           ntohs(addrConfig.sin_port));
  } else {
    if (!remoteConnect(sockOut, addrConfig)) {
      perror("Server: failed remote connection to %s:%d", inet_ntoa(addrConfig.sin_addr),
             ntohs(addrConfig.sin_port));
      return false;
    } else {
      printf("The %s socket will send to %s:%d\n", description.c_str(),
             inet_ntoa(addrConfig.sin_addr), ntohs(addrConfig.sin_port));
    }
  }

  // Return success
  return true;
}

bool RESTServer::connectionReceive() {
  static char buf[4096];
  socklen_t client_address_size = sizeof(client);

  ssize_t recvLen = 0;
  if ((recvLen = recvfrom(sockConn, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&client,
                          &client_address_size))
      < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      perror("Server: error listening for message (%l)", recvLen);
      return false;
    }
  }

  // Process the received message
  if (recvLen > 0) {
    printf("Connection message from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    buf[recvLen] = '\0';  // Null-terminate the string
    printf("\t> %s\n", buf);

    // Parse the message and compute unique address-port key
    ConnectMessage message = makeConnectMessage(buf);
    u64 clientKey = (u64)client.sin_addr.s_addr << 16 | message.port;

    // Handle the connection message
    switch (message.type) {
      case E_TYPE_CONNECT:
        if (streamSockets.find(clientKey) == streamSockets.end()) {
          // Setup the streaming socket for this client
          client.sin_port = htons(message.port);
          if (!initSocket(sockStream, client, "streaming")) {
            close(sockStream);
            sockStream = 0;
            return false;
          }

          // Add the socket to the map
          streamSockets[clientKey] = sockStream;
        } else {
          fprintf(stderr, "Stream already open for %s:%d.\n", inet_ntoa(client.sin_addr),
                  message.port);
        }
        break;

      case E_TYPE_DISCONNECT:
        if (streamSockets.find(clientKey) != streamSockets.end()) {
          // Close the streaming socket for this client
          close(streamSockets[clientKey]);
          streamSockets.erase(clientKey);
        } else {
          fprintf(stderr, "No stream open for %s:%d.\n", inet_ntoa(client.sin_addr), message.port);
          return false;
        }
        break;

      default:
        fprintf(stderr, "Server: invalid connection message received.\n");
        return false;
    }
  }

  return true;
}

bool RESTServer::remoteConnect(int& sockOut, sockaddr_in addrConfig) {
  // Connect ot the host
  int ret = connect(sockOut, (struct sockaddr*)&addrConfig, sizeof(addrConfig));
  if (ret != 0 && errno != EINPROGRESS) {
    perror("Server: remote connection failed (%d).\n", errno);
    return false;
  }

  // Possibly wait for connection to complete
  if (ret != 0) {
    // Setup the socket polling struct
    struct pollfd pfd;
    pfd.fd = sockOut;
    pfd.events = POLLOUT;
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
    perror("Server: failed to send data to %s:%d.\n", inet_ntoa(client.sin_addr),
           ntohs(client.sin_port));
    perror("Server: error code %d.\n", errno);
    return false;
  }

  // Return success
  return true;
}
