/*
 * This program demonstrates an example of nonblocking I/O.  It reads up to
 * 500,000 bytes from STDIO and attempts to write it to STDOUT, which is first
 * set to be nonblocking.  The output is in a loop, with the results of each
 * write() being printed on STDERR.  The function clr_fl() clears one or more
 * of the flag bits.
 */
#include "apue.h"
#include <errno.h>
#include <fcntl.h>

char buf[500000];

int main(void) {
  int ntowrite, nwrite;
  char *ptr;

  ntowrite = read(STDIN_FILENO, buf, sizeof(buf));
  fprintf(stderr, "Read %d bytes\n", ntowrite);

  set_fl(STDOUT_FILENO, O_NONBLOCK); /* set nonblocking */

  ptr = buf;
  while (ntowrite > 0) {
    errno = 0;
    nwrite = write(STDOUT_FILENO, ptr, ntowrite);
    fprintf(stderr, "nwrite = %d, errno = %d\n", nwrite, errno);

    if (nwrite > 0) {
      ptr += nwrite;
      ntowrite -= nwrite;
    }
  }

  clr_fl(STDOUT_FILENO, O_NONBLOCK); /* clear nonblocking */

  exit(0);
}
