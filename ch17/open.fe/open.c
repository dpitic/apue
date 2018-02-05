#include "open.h"
#include <sys/uio.h> /* struct iovec */

/**
 * Implementation of the open server client function that does the fork() and
 * exec() of the server.  Open the file by sending the "name" and "oflag" to the
 * connection server and reading a file descriptor back.  The custom
 * communication protocol between the client and server is:
 *   1. Client sends a request of the form "open <pathname> <openmode>\0"
 *      across the fd-pipe to the server.  The <openmode> is the numeric value
 *      in ASCII decimal, of the second argument to the open() function.  This
 *      request string is terminated by a null byte.
 *   2. The server sends back an open descriptor (by calling send_fd()), or an
 *      error (by calling send_err()).
 * @param name file name to open.
 * @param oflag file mode.
 * @return open descriptor on success; < 0 on error.
 */
int csopen(char *name, int oflag) {
  pid_t pid;
  int len;
  char buf[10];
  struct iovec iov[3];
  static int fd[2] = {-1, -1};

  if (fd[0] < 0) { /* fork()/exec() open server first time */
    if (fd_pipe(fd) < 0) {
      err_ret("fd_pipe() error");
      return (-1);
    }
    if ((pid = fork()) < 0) {
      err_ret("fork() error");
      return (-1);
    } else if (pid == 0) { /* child */
      close(fd[0]);
      if (fd[1] != STDIN_FILENO && dup2(fd[1], STDIN_FILENO) != STDIN_FILENO) {
        err_sys("dup2() error to stdin");
      }
      if (fd[1] != STDOUT_FILENO &&
          dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
        err_sys("dup2() error to stdout");
      }
      if (execl("../opend.fe/opend", "opend", (char *)0) < 0) {
        err_sys("execl() error");
      }
    }
    close(fd[1]); /* parent */
  }
  sprintf(buf, " %d", oflag);    /* oflag to ascii */
  iov[0].iov_base = CL_OPEN " "; /* string contatenation */
  iov[0].iov_len = strlen(CL_OPEN) + 1;
  iov[1].iov_base = name;
  iov[1].iov_len = strlen(name);
  iov[2].iov_base = buf;
  iov[2].iov_len = strlen(buf) + 1; /* +1 for null at end of buf */
  len = iov[0].iov_len + iov[1].iov_len + iov[2].iov_len;
  if (writev(fd[0], &iov[0], 3) != len) {
    err_ret("writev() error");
    return (-1);
  }

  /* Read descriptor, returned errors handled by write() */
  return (recv_fd(fd[0], write));
}
