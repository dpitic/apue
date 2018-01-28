/*
 * Send an error message using a file descriptor.
 */
#include "apue.h"

/**
 * Send an error message using a file descriptor.  Normally used by a server
 * process.  Used when a file descriptor is sent using send_fd() encountered an
 * error.  The error is sent back using the send_fd()/recv_fd() protocol.  This
 * function implements a custom protocol to send an error.  It sends the error
 * message followed by a byte of 0, followed by the absolute value of the status
 * byte (1 through 255).
 * @param fd UNIX domain socket file descriptor used to send the error message.
 * @param status byte following the error message.
 * @param msg error message.
 */
int send_err(int fd, int errcode, const char *msg) {
  int n;

  if ((n = strlen(msg)) > 0) {
    if (writen(fd, msg, n) != n) {		/* send the error message */
      return (-1);
    }
  }

  if (errcode >= 0) {
    errcode = -1; /* must be negative */
  }

  if (send_fd(fd, errcode) < 0) {
    return (-1);
  }

  return (0);
}
