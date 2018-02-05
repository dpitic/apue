#include "opend.h"
#include <fcntl.h>

/**
 * Open server request handling function.
 * Implements the open server request logic to process a client's request and
 * send back the descriptor across the fd-pipe (its standard output).  If an
 * error is encountered, it sends back an error message, using the custom
 * defined client-server protocol.
 */
void handle_request(char *buf, int nread, int fd) {
  int newfd;

  if (buf[nread - 1] != 0) {
    snprintf(errmsg, MAXLINE - 1, "Request not null terminated: %*.*s\n", nread,
             nread, buf);
    send_err(fd, -1, errmsg);
    return;
  }
  if (buf_args(buf, cli_args) < 0) { /* parse args & set options */
    send_err(fd, -1, errmsg);
    return;
  }
  if ((newfd = open(pathname, oflag)) < 0) {
    snprintf(errmsg, MAXLINE - 1, "Can't open %s: %s\n", pathname,
             strerror(errno));
    send_err(fd, -1, errmsg);
    return;
  }
  if (send_fd(fd, newfd) < 0) { /* send the descriptor */
    err_sys("send_fd() error");
  }
  close(newfd); /* finished with the descriptor */
}
