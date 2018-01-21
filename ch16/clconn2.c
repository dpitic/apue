/*
 * This is a portable implementation of the connection retry function.  It
 * needs to close the socket if connect() fails, and a new socket has to be
 * opened if the connection needs to be retried.
 */
#include "apue.h"
#include <sys/socket.h>

#define MAXSLEEP 128

int connect_retry(int domain, int type, int protocol,
                  const struct sockaddr *addr, socklen_t alen) {
  int numsec, fd;

  /*
   * Try to connect with exponential backoff.
   */
  for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
    if ((fd = socket(domain, type, protocol)) < 0) {
      return (-1);
    }
    if (connect(fd, addr, alen) == 0) {
      /*
       * Connection accepted.
       */
      return (fd);
    }

    /*
     * Connection not accepted.  Cannot reuse socket descriptor (for portability
     * reasons).
     */
    close(fd);

    /*
     * Delay before trying again.
     */
    if (numsec <= MAXSLEEP / 2) {
      sleep(numsec);
    }
  }
  return (-1);
}
