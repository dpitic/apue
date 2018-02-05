/*
 * Open Server, Version 1
 * This is an open server implemented using file descriptor passing.  It is a
 * program that is executed by a process to open one or more files.  Instead
 * of sending the contents of the file back to the calling process, this
 * server sends back an open file descriptor.  This technique enables the open
 * server to work with any type of file (such as a device or socket), and not
 * simply just regular files.  The client and server exchange a minimum amount
 * of information using IPC: the filename and open mode sent by the client, and
 * the descriptor returned by the server.  The contents of the file are not
 * exchanged using IPC.  This version of the server is designed to be a
 * separate executable program, one that is executed by the client.
 *
 * The main() function is a loop that reads a pathname from standard input and
 * copies the file to standard output.  It calls csopen() to contact the open
 * server and return an open descriptor.
 */
#include "open.h"
#include <fcntl.h>

#define BUFFSIZE 8192

int main(int argc, char const *argv[]) {
  int n, fd;
  char buf[BUFFSIZE];
  char line[MAXLINE];

  /* Read filename to cat from stdin */
  while (fgets(line, MAXLINE, stdin) != NULL) {
    if (line[strlen(line) - 1] == '\n') {
      line[strlen(line) - 1] = 0; /* replace newline with null */
    }

    /* Open the file */
    if ((fd = csopen(line, O_RDONLY)) < 0) {
      continue; /* csopen() prints error from server */
    }

    /* And cat to stdout */
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
