/*
 * Receive a file descriptor.
 */
#include "apue.h"
#include <sys/socket.h> /* struct msghdr */

/* Size of control buffer to send/recv one file descriptor */
#define CONTROLLEN CMSG_LEN(sizeof(int))

static struct cmsghdr *cmptr = NULL; /* malloc'ed first time */

/**
 * Receive a file descriptor from a server process.  Any data received is passed
 * to a user function.  This function uses a custom protocol.  It reads
 * everything on the socket until it encounters a null byte.  Any characters
 * read up to this point are passed to the callers user function.  The next byte
 * read is the status byte.  If it is 0, a descriptor was passed; otherwise,
 * there is no descriptor to receive.
 * @param fd UNIX domain socket file descriptor used to receive the file
 * descriptor send by the server.
 * @param userfunc user defined function that gets passed any data received.
 * @return file descriptor send by server if OK, negative value on error.
 */
int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t)) {
  int newfd, nr, status;
  char *ptr;
  char buf[MAXLINE];
  struct iovec iov[1];
  struct msghdr msg;

  status = -1;
  for (;;) {
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL) {
      return (-1);
    }
    msg.msg_control = cmptr;
    msg.msg_controllen = CONTROLLEN;
    if ((nr = recvmsg(fd, &msg, 0)) < 0) {
      err_ret("recvmsg() error");
      return (-1);
    } else if (nr == 0) {
      err_ret("Connection closed by server");
      return (-1);
    }

    /*
     * See if this is the final data with null & status.  Null is next to last
     * byte of buffer; status byte is last byte.  Zero status means there is a
     * file descriptor to receive.
     */
    for (ptr = buf; ptr < &buf[nr];) {
      if (*ptr++ == 0) {
        if (ptr != &buf[nr - 1]) {
          err_dump("Message format error");
        }
        status = *ptr & 0xFF; /* prevent sign extension */
        if (status == 0) {
          if (msg.msg_controllen != CONTROLLEN) {
            err_dump("status = 0 but no fd");
          }
          newfd = *(int *)CMSG_DATA(cmptr);
        } else {
          newfd = -status;
        }
        nr -= 2;
      }
    }
    if (nr > 0 && (*userfunc)(STDERR_FILENO, buf, nr) != nr) {
      return (-1);
    }
    if (status >= 0) { /* final data has arrived */
      return (newfd);  /* descriptor, or -status */
    }
  }
}
