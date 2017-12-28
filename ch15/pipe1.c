/*
 * This program demonstrates how to create a pipe between a parent and child
 * process and to send data down the pipe.  The parent process writes data to
 * the pipe, and the child process reads the data from the pipe and displays it
 * on the standard output.
 */
#include "apue.h"

int main(void) {
  int n;              /* number of bytes read & written */
  int fd[2];          /* pipe file descriptor array */
  pid_t pid;          /* parent/child pid */
  char line[MAXLINE]; /* read buffer */

  /* Create pipe (descriptor pair) for IPC */
  if (pipe(fd) < 0) {
    err_sys("pipe() error");
  }

  if ((pid = fork()) < 0) {
    err_sys("fork() error");
  } else if (pid > 0) { /* parent writes to the pipe */
    close(fd[0]); /* read pipe fd */
    write(fd[1], "hello world\n", 12);
  } else {        /* child reads from the pipe and outputs to stdout */
    close(fd[1]); /* write pipe fd */
    n = read(fd[0], line, MAXLINE);
    write(STDOUT_FILENO, line, n);
  }
  exit(0);
}
