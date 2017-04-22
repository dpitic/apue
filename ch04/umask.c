/*
 * This program demonstrates the umask() function, which sets the file mode
 * creation mask (umask) for the calling process. Any bits that are ON in the
 * file mode creation mask (umask) are turned OFF in the file's mode. Changing
 * the file mode creation mask of a process doesn't affect the mask of its
 * parent (often a shell).
 *
 * This program creates two files: one with a umask of 0 and one with a umask
 * that disables all the group and permission bits.
 */
#include "apue.h"
#include <fcntl.h>

#define RWRWRW (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int main(void) {
  umask(0);
  if (creat("foo", RWRWRW) < 0) {
    err_sys("creat error for foo");
  }
  umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (creat("bar", RWRWRW) < 0) {
    err_sys("creat error for bar");
  }
  exit(0);
}
