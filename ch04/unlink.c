/*
 * This program opens a file and then unlinks it. The program then goes to
 * sleep for 15 seconds before terminating.
 */
#include "apue.h"
#include <fcntl.h>

int main(void) {
  int fd;
  if ((fd = open("tempfile", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
    err_sys("open error");
  }
  printf("tempfile fd = %d\n", fd);
  if (write(fd, "hello", 5) != 5) {
    err_sys("write error");
  }
  if (unlink("tempfile") < 0) {
    err_sys("unlink error");
  }
  printf("file unlinked\n");
  sleep(15);
  printf("done\n");
  exit(0);
}