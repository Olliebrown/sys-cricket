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

  // Clear the socket handle and port
  sockConn = 0;
}

bool RESTServer::threadInit() {
  // Initialize timer and event handles
  utimerCreate(&connectTimer, 100000000, TimerType_Repeating);  // 1/10s
  ueventCreate(&exitEvent, false);
  waiters.push_back(waiterForUTimer(&connectTimer));
  waiters.push_back(waiterForUEvent(&exitEvent));

  // Initialize the connection socket
  if (!initConnectionSocket()) {
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

  // Close and clear all sockets
  if (sockConn > 0) {
    close(sockConn);
  }
  sockConn = 0;

  // Close out all running stream sockets
  for (auto& stream : streams) {
    delete stream.second;
  }
  streams.clear();
}

// Server control function
void RESTServer::serverMain() {
  utimerStart(&connectTimer);
}

bool RESTServer::initConnectionSocket() {
  // Create a UDP socket
  sockConn = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockConn < 0) {
    perror("Server: failed to create connection socket");
    return false;
  }

  // Set to non-blocking mode
  int flags = fcntl(sockConn, F_GETFL, 0);
  if (flags == -1) {
    perror("Server: can't set non-blocking / failed to read connection socket flags.\n");
  } else if (fcntl(sockConn, F_SETFL, flags | O_NONBLOCK) != 0) {
    perror("Server: failed to set connection socket to non-blocking io.\n");
  }

  // Bind to a local port
  if (bind(sockConn, (struct sockaddr*)&server, sizeof(server)) < 0) {
    perror("Server: failed to bind connection socket");
    return false;
  }

  // Output the port number
  printf("The connection socket is listening on port %d\n", ntohs(server.sin_port));

  // Return success
  return true;
}

bool RESTServer::connectionReceive() {
  static char buf[4096];
  struct sockaddr_in client;
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
        if (streams.find(clientKey) == streams.end()) {
          // Setup the streaming socket for this client
          client.sin_port = htons(message.port);
          StreamSession* sockStream = new StreamSession(client);

          // Add the socket to the map
          streams[clientKey] = sockStream;
        } else {
          fprintf(stderr, "Stream already open for %s:%d.\n", inet_ntoa(client.sin_addr),
                  message.port);
        }
        break;

      case E_TYPE_DISCONNECT:
        if (streams.find(clientKey) != streams.end()) {
          // Close the streaming socket for this client
          delete streams[clientKey];
          streams.erase(clientKey);
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
