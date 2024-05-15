#include "socketLogging.h"

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

static FILE* fileHandle = nullptr;
int redirectOutputToFile(const char* filepath, bool append)
{
  // Open file for output
  fileHandle = fopen(filepath, (append ? "a" : "w"));
  if (fileHandle == nullptr) {
    fprintf(stderr, "File Logging: Failed to open %s for redirecting stdout and stderr.\n", filepath);
    return -1;
  }

  // Get the file descriptor for use with dup
  int fd = fileno(fileHandle);

  // Redirect stdout to the file
  fflush(stdout);
  if (dup2(fd, STDOUT_FILENO) < 0) {
    fprintf(stderr, "File Logging: Failed redirect stdout to %s.\n", filepath);
    fclose(fileHandle);
    fileHandle = nullptr;
    return -2;
  }

  fprintf(stdout, "File Logging: stdout redirected to %s.\n", filepath);

  // Redirect stderr to the file
  fflush(stderr);
  if (dup2(fd, STDERR_FILENO) < 0) {
    fprintf(stderr, "File Logging: Failed redirect stderr to %s.\n", filepath);
    fclose(fileHandle);
    fileHandle = nullptr;
    return -3;
  }

  fprintf(stderr, "File Logging: stderr redirected to %s.\n", filepath);

  // Return the fileHandle
  return fd;
}
