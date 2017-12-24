/*
 * This function is used to read a specified number of bytes and handle a
 * return value that's possibly less than requested.  The implementation simply
 * calls read() as many times as required to read the entire amount of data
 * requested.
 */
#include "apue.h"

/*
 * Read n bytes from a descriptor and return the number of bytes read.
 */
ssize_t readn(int fd, void *ptr, size_t n) {
  size_t nleft;
  ssize_t nread;

  nleft = n;
  while (nleft > 0) {
    if ((nread = read(fd, ptr, nleft)) < 0) {
      if (nleft == n) {
        return (-1); /* error; return -1 */
      } else {
        break; /* error, return amount read so far */
      }
    } else if (nread == 0) {
      break; /* EOF */
    }
    nleft -= nread;
    ptr += nread;
  }
  return (n - nleft); /* return >= 0 */
}
