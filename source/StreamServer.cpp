#include "StreamServer.h"

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

StreamServer::StreamServer() : ThreadedServer(false) {
  // Set up the connection listening address
  server.sin_family = AF_INET;
  server.sin_port = htons(LISTENING_SOCKET_PORT);
  server.sin_addr.s_addr = INADDR_ANY;

  // Clear the socket handle and port
  sockConn = 0;
}

bool StreamServer::threadInit() {
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

void StreamServer::threadMain() {
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
          index -= 2;
          if (index >= 0 && index < (int)streamKeys.size()) {
            fprintf(stdout, "Reading and sending data for stream %d\n", index);
            fprintf(stdout, "> ClientKey: %s\n", streamKeys[index].c_str());
            streams[streamKeys[index]]->readAndSendData();
          } else {
            fprintf(stdout, "Unknown event index %d\n", index);
          }
          break;
      }
    }
  }
}

// Send the thread the exit event
void StreamServer::threadExit() {
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
void StreamServer::serverMain() {
  utimerStart(&connectTimer);
}

bool StreamServer::initConnectionSocket() {
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

bool StreamServer::connectionReceive() {
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
    printf("Config message from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    buf[recvLen] = '\0';  // Null-terminate the string
    printf("\t> %s\n", buf);

    // Parse the message and compute unique address-port key
    ConfigMessage message(buf);
    client.sin_port = htons(message.port);
    std::string parentKey = StreamSession::getClientKey(client, "");
    std::string clientKey = StreamSession::getClientKey(client, message.nickname);

    // Handle the connection message
    return handleConnectionMessage(client, message, parentKey, clientKey);
  }

  // If no message was received, just return true
  return true;
}

bool StreamServer::handleConnectionMessage(struct sockaddr_in client, const ConfigMessage& message,
                                           std::string parentKey, std::string clientKey) {
  // Handle the connection message
  switch (message.messageType) {
    case eConfigType_Connect:
      if (streams.find(parentKey) == streams.end()) {
        // Setup the streaming socket for this client
        DataSession* sockStream = new DataSession(client, message);
        fprintf(stdout, "Parent StreamSession created for %s\n", parentKey.c_str());

        // Add the socket to the map
        streams[parentKey] = sockStream;

        // Send status message then start the stream
        sockStream->streamSendStatus();
      } else {
        fprintf(stderr, "StreamSession already open for %s:%d.\n", inet_ntoa(client.sin_addr),
                message.port);
        return false;
      }
      break;

    case eConfigType_StartData:
      if (streams.find(parentKey) != streams.end()) {
        // Setup the streaming socket for this client
        int parentSocket = streams[parentKey]->useSocket();
        DataSession* sockStream = new DataSession(client, message, parentSocket);
        fprintf(stdout, "DataSession created for %s\n", clientKey.c_str());

        // Start the stream for this client
        waiters.push_back(sockStream->startStream(message.nsInterval));
        streams[clientKey] = sockStream;
        streamKeys.push_back(clientKey);
      } else {
        fprintf(stderr, "Host not connected (%s:%d).\n", inet_ntoa(client.sin_addr), message.port);
        return false;
      }
      break;

    case eConfigType_StopData:
      if (streams.find(clientKey) != streams.end()) {
        streams[parentKey]->freeSocket();

        // Close the streaming socket for this client
        delete streams[clientKey];
        streams.erase(clientKey);

        // Delete matching stream and waiters from streamList
        for (int i = 0; i < (int)streamKeys.size(); i++) {
          if (streamKeys[i] == clientKey) {
            streamKeys.erase(streamKeys.begin() + i);
            waiters.erase(waiters.begin() + i + 2);
            break;
          }
        }
      } else {
        fprintf(stderr, "No data stream for %s.\n", clientKey.c_str());
        return false;
      }
      break;

    case eConfigType_Disconnect:
      if (streams.find(parentKey) != streams.end()) {
        // Is this parent stream still in use?
        if (streams[parentKey]->getUseCount() > 0) {
          fprintf(stderr, "Parent StreamSession still in use for %s:%d.\n",
                  inet_ntoa(client.sin_addr), message.port);
          return false;
        }

        // Close the streaming socket for this client
        delete streams[parentKey];
        streams.erase(parentKey);
      } else {
        fprintf(stderr, "No stream open for %s:%d.\n", inet_ntoa(client.sin_addr), message.port);
        return false;
      }
      break;

    default:
      fprintf(stderr, "Server: invalid config message received.\n");
      return false;
  }

  return true;
}
