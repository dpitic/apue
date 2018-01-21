/*
 * This function demonstrates one way to handle transient connect() errors.  It
 * implements an exponential backoff algorithm.  If the call to connect() fails,
 * the process goes to sleep for a short time and then tries again, increasing
 * the delay each time, up to a maximum delay of about 2 minutes.
 *
 * WARNING: This code is not portable.
 *
 * The technique implemented here works on Linux and Solaris, but doesn't work
 * as expected on FreeBSD and macOS.  If the first connection attempt fails,
 * BSD-based socket implementations continue to fail successive connection
 * attempts when the same socket descriptor is used with TCP.  This is a case
 * of a protocol specific behaviour leaking through the (protocol independent)
 * socket interface and becoming visible to applications.  The SUS warns that
 * the state of a socket is undefined if connect() fails.
 */
#include "apue.h"
#include <sys/socket.h>

#define MAXSLEEP 128

int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen) {
  int numsec;

  /*
   * Try to connect with exponential backoff.
   */
  for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
    if (connect(sockfd, addr, alen) == 0) {
      /*
       * Connection accepted.
       */
      return (0);
    }

    /*
     * Delay before trying again.
     */
    if (numsec <= MAXSLEEP / 2) {
      sleep(numsec);
    }
  }
  return (-1);
}
