#include <switch.h>

// This class is a base class for creating a server that runs in its own thread
class ThreadedServer {
public:
  ThreadedServer(bool newAutoExit = true) : autoExit(newAutoExit) {}
  virtual ~ThreadedServer() {}

  // Attempt to start the server (only one server can be running at a time)
  bool startServer();

  // Print to stdout without interfering with other threads
  static void printf(const char* fmt, ...);
  static void perror(const char* fmt, ...);

protected:
  // Should the thread be automatically exited when the main function returns
  bool autoExit;

  // Main thread function
  virtual bool threadInit() = 0;
  virtual void threadMain() = 0;
  virtual void threadExit() = 0;

  // Server control functions
  virtual void serverMain() = 0;

  // Internal main loop to start and run the server
  int runServerThread();

  // Only one server/thread can be running at a time so track as static variables
  static ThreadedServer* runningServer;
  static Thread* runningThread;

  // Wrapper to start the main thread method
  static void threadWrapper(void* arg);
};
