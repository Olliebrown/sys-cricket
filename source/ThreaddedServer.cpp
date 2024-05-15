#include "ThreaddedServer.h"

#include <cstdarg>
#include <cstdio>

// Printf with mutex to avoid thread issues
void ThreadedServer::printf(const char* fmt, ...)
{
  static Mutex printMutex = INVALID_HANDLE;
  mutexLock(&printMutex);

  va_list va;
  va_start(va, fmt);
  vfprintf(stdout, fmt, va);
  va_end(va);

  mutexUnlock(&printMutex);
}

void ThreadedServer::perror(const char* fmt, ...)
{
  static Mutex errorMutex = INVALID_HANDLE;
  mutexLock(&errorMutex);

  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);

  mutexUnlock(&errorMutex);
}

// Static variables to track running server and thread
ThreadedServer* ThreadedServer::runningServer = nullptr;
Thread* ThreadedServer::runningThread = nullptr;

// Thread function to handle timer and exit events
void ThreadedServer::threadWrapper(void* arg)
{ 
  if (runningServer != nullptr) {
    runningServer->threadMain();
  }
}

bool ThreadedServer::startServer() {
  // Only one server can be running at a time
  if (runningServer != nullptr) {
    fprintf(stderr, "Server: already running\n");
    return false;
  }

  // Run the server thread
  runningServer = this;
  int result = runServerThread();
  runningServer = nullptr;
  return (result == 0);
}

// Main event loop
int ThreadedServer::runServerThread () {
  // Make sure only one server thread is running
  if (runningThread != nullptr) {
    fprintf(stderr, "Server: thread already running\n");
    return -1;
  }

  // Initialize server
  threadInit();

  // Create thread
  Result rc;
  Thread thread;

  ThreadedServer::printf("Creating thread\n");
  rc = threadCreate(&thread, ThreadedServer::threadWrapper, NULL, NULL, 0x10000, 0x2C, -2);

  // If successfully created, start the thread
  if (R_SUCCEEDED(rc)) {
    runningThread = &thread;

    // Attempt to start the thread
    rc = threadStart(&thread);
    if (R_SUCCEEDED(rc)) {
      // Run the server control function
      printf("Running server control thread...\n");
      serverMain();

      // Wait for the thread to exit
      if (autoExit) { threadExit(); }
      printf("Waiting for server thread to exit...\n");
      threadWaitForExit(&thread); 
    }

    // Close the thread
    threadClose(&thread);
  }

  // Clean up
  printf("Server stopped.\n");
  runningThread = nullptr;
  return 0;
}
