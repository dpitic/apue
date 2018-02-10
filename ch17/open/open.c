#include "open.h"
#include <sys/uio.h> /* struct iovec */

/**
 * Implementation of the open server client function that opens the file by
 * sending the name and oflag to the connection server and reading a file
 * descriptor back.
 * @param name of file to open.
 * @param oflag file mode.
 * @return open descriptor on success; < 0 on error.
 */
int csopen(char *name, int oflag) {
  int len;
  char buf[12];
  struct iovec iov[3];
  static int csfd = -1;

  if (csfd < 0) { /* open connection to connection server */
    if ((csfd = cli_conn(CS_OPEN)) < 0) {
      err_ret("cli_conn() error");
      return (-1);
    }
  }

  sprintf(buf, " %d", oflag);    /* oflag to ascii */
  iov[0].iov_base = CL_OPEN " "; /* string concatenation */
  iov[0].iov_len = strlen(CL_OPEN) + 1;
  iov[1].iov_base = name;
  iov[1].iov_len = strlen(name);
  iov[2].iov_base = buf;
  iov[2].iov_len = strlen(buf) + 1; /* null always sent */
  len = iov[0].iov_len + iov[1].iov_len + iov[2].iov_len;
  if (writev(csfd, &iov[0], 3) != len) {
    err_ret("writev() error");
    return (-1);
  }

  /* Read back descriptor; returned errors handled by write() */
  return (recv_fd(csfd, write));
}
