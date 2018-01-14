/*
 * This program uses the special /dev/zero device.  The /dev/zero device is an
 * infinite source of 0 bytes when read.  This device also accepts any data that
 * is written to it, ignoring the data.  For IPC, this device has special
 * properties when it is memory mapped:
 *   * An unnamed memory region is created whose size is the second argument to
 *     mmap(), rounded up to the nearest page size on the system.
 *   * The memory region is initialised to 0.
 *   * Multiple processes can share this region if a common ancestor specifies
 *     the MAP_SHARED flag to mmap().
 */
#include "apue.h"
#include <fcntl.h>
#include <sys/mman.h>

#define NLOOPS 1000
#define SIZE sizeof(long) /* size of shared memory area */

static int update(long *ptr) {
  return ((*ptr)++); /* return value before increment */
}

int main(void) {
  int fd, i, counter;
  pid_t pid;
  void *area;

  if ((fd = open("/dev/zero", O_RDWR)) < 0) {
    err_sys("open() error");
  }
  if ((area = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) ==
      MAP_FAILED) {
    err_sys("mmap() error");
  }
  close(fd); /* can close /dev/zero now that it's mapped */

  TELL_WAIT();

  if ((pid = fork()) < 0) {
    err_sys("fork() error");
  } else if (pid > 0) { /* parent */
    for (i = 0; i < NLOOPS; i += 2) {
      if ((counter = update((long *)area)) != i) {
        err_quit("Parent: expected %d, got %d", i, counter);
      }

      TELL_CHILD(pid);
      WAIT_CHILD();
    }
  } else { /* child */
    for (i = 1; i < NLOOPS + 1; i += 2) {
      WAIT_PARENT();

      if ((counter = update((long *)area)) != i) {
        err_quit("Child: expected %d, got %d", i, counter);
      }

      TELL_PARENT(getppid());
    }
  }

  exit(0);
}
