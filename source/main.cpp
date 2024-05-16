#include <cstdio>

#include <switch.h>

#include "RESTServer.h"
#include "fileLogging.h"
#include "socketLogging.h"

// Constants for redirecting stdio
#define STDIO_FILE_PATH "sdmc:/config/sys-cricket/stdio.log"

#ifndef STDIO_SOCKET_HOST
#define STDIO_SOCKET_HOST "192.168.50.226"
#endif

#ifndef STDIO_SOCKET_PORT
#define STDIO_SOCKET_PORT 42424
#endif

// libnx fake heap initialization
extern "C" {

#define HEAP_SIZE 0x000340000

// we aren't an applet
u32 __nx_applet_type = AppletType_None;

// setup a fake heap (we don't need the heap anyway)
char fake_heap[HEAP_SIZE];

// we override libnx internals to do a minimal init
void __libnx_initheap(void) {
  extern char* fake_heap_start;
  extern char* fake_heap_end;

  // setup newlib fake heap
  fake_heap_start = fake_heap;
  fake_heap_end = fake_heap + HEAP_SIZE;
}

void __appInit(void) {
  /* Initialize services */
  nsInitialize();
  smInitialize();
  fsInitialize();
  fsdevMountSdmc();
  timeInitialize();
  socketInitializeDefault();
}

void __appExit(void) {
  fsdevUnmountAll();
  fsExit();
  timeExit();
  socketExit();
  smExit();
  nsExit();
}

}  // extern "C"

int main() {
  // Redirect stdio to log to files
  int stdioFile = redirectOutputToFile(STDIO_FILE_PATH, false);
  if (stdioFile < 0) {
    fprintf(stderr, "Error: failed to redirect stdio to file. (%s, %d)\n", STDIO_FILE_PATH,
            stdioFile);
  }

  // Duplicate stdio to a tcp socket host
  int socket = redirectOutputToSockets(STDIO_SOCKET_HOST, STDIO_SOCKET_PORT);
  if (socket < 0) {
    fprintf(stderr, "Error: failed to connect to logging socket host. (%s:%d, %d)\n",
            STDIO_SOCKET_HOST, STDIO_SOCKET_PORT, socket);
  }

  // Start the REST server
  RESTServer* server = new RESTServer();
  server->startServer();
  return 0;
}
