/*
 * This program demonstrates and example of binding an address to a UNIX domain
 * socket.
 */
#include "apue.h"
#include <sys/socket.h>
#include <sys/un.h>

int main(void)
{
  int fd, size;
  struct sockaddr_un un;

  un.sun_family = AF_UNIX;
  strcpy(un.sun_path, "foo.socket");
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    err_sys("socket() failed");
  }
  /*
   * Determine the size of the address to bind by calculating the offset of the
   * sun_path member in the socketaddr_un structure, and add it to the length of
   * the pathname, not including the terminating null byte.  Since different
   * implementations vary in which members precede sun_path in the socketaddr_un
   * structure, use the offsetof() macro defined in <stddef.h> to calculate the
   * offset of the sun_path member from the start of the structure.
   */
  size = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
  if (bind(fd, (struct sockaddr *)&un, size) < 0) {
    err_sys("bind() failed");
  }
  printf("UNIX domain socket bound\n");
  exit(0);
}
