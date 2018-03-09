/*
 * do_driver() function called by pty program.
 */
#include "apue.h"

/**
 * Driver process for input and output.  The standard output of the driver is
 * pty's standard input, and vice versa.  This implementation uses a single
 * bidirectional pipe between pty and the driver process.
 * @param driver pointer to driver process pathname.
 */
void do_driver(char *driver) {
  pid_t child;
  int pipe[2];

  /*
   * Create a full-duplex pipe to communicate with the driver.
   */
  if (fd_pipe(pipe) < 0) {
    err_sys("Can't create stream pipe");
  }

  if ((child = fork()) < 0) {
    err_sys("fork() error");
  } else if (child == 0) {    /* child */
    close(pipe[1]);

    /* stdin for driver */
    if (dup2(pipe[0], STDIN_FILENO) != STDIN_FILENO) {
      err_sys("dup2() error to stdin");
    }

    /* stdout for driver */
    if (dup2(pipe[0], STDOUT_FILENO) != STDOUT_FILENO) {
      err_sys("dup2() error to stdout");
    }
    if (pipe[0] != STDIN_FILENO && pipe[0] != STDOUT_FILENO) {
      close(pipe[0]);
    }

    /* Leave stderr for driver alone */
    execlp(driver, driver, (char *)0);
    err_sys("execlp() error for %s:", driver);
  }

  close(pipe[0]);   /* parent */
  if (dup2(pipe[1], STDIN_FILENO) != STDIN_FILENO) {
    err_sys("dup2() error to stdin");
  }
  if (dup2(pipe[1], STDOUT_FILENO) != STDOUT_FILENO) {
    err_sys("dup2() error to stdout");
  }
  if (pipe[1] != STDIN_FILENO && pipe[1] != STDOUT_FILENO) {
    close(pipe[1]);
  }

  /*
   * Parent returns, but with stdin and stdout connected to the driver.
   */
}

