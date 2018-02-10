/*
 * Open Server, Version 2
 * This is the client application for the Open Server, Version 2.  The Open
 * Server is implemented as a daemon process, where one server handles all
 * clients.  Compared to version 1, which was implemented using fork() and
 * exec(), this version design is expected to be more efficient.  UNIX domain
 * socket connections between client and server are used to pass file
 * descriptors (between unrelated processes).
 */
#include "open.h"
#include <fcntl.h>

#define BUFFSIZE 8192

int main(int argc, char *argv[]) {
  int n, fd;
  char buf[BUFFSIZE], line[MAXLINE];

  /* Read filename to cat from stdin */
  while (fgets(line, MAXLINE, stdin) != NULL) {
    if (line[strlen(line) - 1] == '\n') {
      line[strlen(line) - 1] = 0; /* replace newline with null */
    }

    /* Open the file */
    if ((fd = csopen(line, O_RDONLY)) < 0) {
      continue; /* csopen() prints error from server */
    }

    /* Cat to stdout */
    while ((n = read(fd, buf, BUFFSIZE)) > 0) {
      if (write(STDOUT_FILENO, buf, n) != n) {
        err_sys("write() error");
      }
    }
    if (n < 0) {
      err_sys("read() error");
    }
    close(fd);
  }
  exit(0);
}
