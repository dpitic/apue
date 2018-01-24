/*
 * This function demonstrates how to allocate and initialise a socket for use
 * by a server process.  It is an improvement over initsrv1, which fails to
 * operate properly when the server terminates and we try to restart it
 * immediately.  Normally, the implementation of TCP will prevent us from
 * binding the same address until a timeout expires, which is usually on the
 * order of several minutes.  Luckily, the SO_REUSEADDR socket option allows
 * us to bypass this restriction.
 */
#include "apue.h"
#include <errno.h>
#include <sys/socket.h>

int initserver(int type, const struct sockaddr *addr, socklen_t alen,
               int qlen) {
  int fd, err;
  int reuse = 1;

  if ((fd = socket(addr->sa_family, type, 0)) < 0) {
    return (-1);
  }
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
    goto errout;
  }
  if (bind(fd, addr, alen) < 0) {
    goto errout;
  }
  if (type == SOCK_STREAM || type == SOCK_SEQPACKET) {
    if (listen(fd, qlen) < 0) {
      goto errout;
    }
  }
  return (fd);

errout:
  err = errno;
  close(fd);
  errno = err;
  return (-1);
}
