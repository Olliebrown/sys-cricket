#include "socketLogging.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <unistd.h>

static int sock = -1;
int redirectOutputToSockets(const char* hostAddress, uint16_t hostPort)
{
  // Create new socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr, "Socket Logging: failed to open socket.\n");
    return -1;
  }

  // set to non-blocking
  int flags = fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    close(sock);
    sock = -1;
    fprintf(stderr, "Socket Logging: failed to read socket flags.\n");
    return -2;
  }

  if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) != 0) {
    close(sock);
    sock = -1;
    fprintf(stderr, "Socket Logging: failed to set socket to non-blocking io.\n");
    return -3;
  }

  // Build server address structure
  struct sockaddr_in srv_addr;
  bzero(&srv_addr, sizeof srv_addr);
  srv_addr.sin_family = AF_INET;
  srv_addr.sin_port = htons(hostPort);

  if(inet_pton(AF_INET, hostAddress, &srv_addr.sin_addr) <= 0) {
    close(sock);
    sock = -1;
    fprintf(stderr, "Socket Logging: failed to convert %s to INET address.\n", hostAddress);
    return -4;
  }

  // Connect ot the host
  int ret = connect(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
  if (ret != 0 && errno != EINPROGRESS) {
    close(sock);
    sock = -1;
    fprintf(stderr, "Socket Logging: failed to connect to %s on port %d.\n", hostAddress, hostPort);
    return -5;
  }

  // Possibly wait for connection to complete
  if (ret != 0) {
    // Setup the socket polling struct
    struct pollfd pfd;
    pfd.fd      = sock;
    pfd.events  = POLLOUT;
    pfd.revents = 0;

    // Wait up to 1s to connect
    int n = poll(&pfd, 1, 1000);
    if (n < 0) {
      close(sock);
      sock = -1;
      fprintf(stderr, "Socket Logging: failed to wait for socket to connect.\n");
      return -6;
    }

    // Timed out
    if (n == 0 || !(pfd.revents & POLLOUT)) {
      close(sock);
      sock = -1;
      errno = ETIMEDOUT;
      fprintf(stderr, "Socket Logging: timed out waiting for socket to connect.\n");
      return -7;
    }
  }

  // Reset back to blocking
  if (fcntl(sock, F_SETFL, flags & ~O_NONBLOCK) != 0) {
    close(sock);
    sock = -1;
    fprintf(stderr, "Socket Logging: filed to set socket back to blocking IO.\n");
    return -8;
  }

  // Redirect stdout to the socket
  fflush(stdout);
  if (dup2(sock, STDOUT_FILENO) < 0) {
    close(sock);
    sock = -1;
    fprintf(stdout, "Socket Logging: Failed to redirect stdout to %s on port %d.\n", hostAddress, hostPort);
    return -1;
  }
  fprintf(stdout, "Socket Logging: stdout redirected to %s on port %u.\n", hostAddress, hostPort);

  // Redirect stderr to the socket
  fflush(stderr);
  if (dup2(sock, STDERR_FILENO) < 0) {
    close(sock);
    sock = -1;
    fprintf(stderr, "Socket Logging: Failed to redirect stderr to %s on port %d.\n", hostAddress, hostPort);
    return -1;
  }
  fprintf(stderr, "Socket Logging: stderr redirected to %s on port %u.\n", hostAddress, hostPort);

  // Return the socket
  return sock;
}
