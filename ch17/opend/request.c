#include "opend.h"
#include <fcntl.h>

/**
 * Open Server request handling function for daemon server, which logs error
 * messages instead of printing them on stderr.  This function implements the
 * Open Server request logic to process a client's request and send back the
 * descriptor across the fd-pipe (its standard output).  If an error is
 * encountered, it sends back an error message, using the custom defined
 * client-server protocol.
 */
void handle_request(char *buf, int nread, int clifd, uid_t uid) {
  int newfd;

  if (buf[nread - 1] != 0) {
    snprintf(errmsg, MAXLINE - 1,
             "Request from uid %d not null terminated: %*.*s\n", uid, nread,
             nread, buf);
    send_err(clifd, -1, errmsg);
    return;
  }
  log_msg("Request: %s, from uid %d", buf, uid);

  /* Parse the arguments, set options */
  if (buf_args(buf, cli_args) < 0) {
    send_err(clifd, -1, errmsg);
    log_msg(errmsg);
    return;
  }

  if ((newfd = open(pathname, oflag)) < 0) {
    snprintf(errmsg, MAXLINE - 1, "Can't open %s: %s\n", pathname,
             strerror(errno));
    send_err(clifd, -1, errmsg);
    log_msg(errmsg);
    return;
  }

  /* Send the descriptor */
  if (send_fd(clifd, newfd) < 0) {
    log_sys("send_fd() error");
  }
  log_msg("Sent fd %d over fd %d for %s", newfd, clifd, pathname);
  close(newfd);
}
