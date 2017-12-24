/*
 * This function is used to write a specified number of bytes and handle a
 * return value that's possibly less than requested.  The implementation simply
 * calls write() as many times a required to write the entire amount of data
 * requested.
 */
#include "apue.h"

/*
 * Write n bytes to a descriptor and return the number of bytes written.
 */
ssize_t writen(int fd, const void *ptr, size_t n) {
  size_t nleft;
  ssize_t nwritten;

  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) < 0) {
      if (nleft == n) {
        return (-1); /* error, return -1 */
      } else {
        break; /* error, return amount written so far */
      }
    } else if (nwritten == 0) {
      break;
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return (n - nleft); /* return >= 0 */
}
