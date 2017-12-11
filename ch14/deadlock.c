/*
 * A deadlock occurs when two processes are each waiting for a resource that the
 * other has locked.  The potential for a deadlock exists if a process that
 * controls a locked region is put to sleep when it tiries to lock another
 * region that is controlled by a different process.
 *
 * This program demonstrates an example of a desdlock.  The child locks byte 0
 * and the parent locks byte 1.  Each then tries to lock the other's already
 * locked byte.
 */
#include "apue.h"
#include <fcntl.h>

/* Get a write lock for the offset  */
static void lock_a_byte(const char *name, int fd, off_t offset) {
  if (writew_lock(fd, offset, SEEK_SET, 1) < 0) {
    err_sys("%s: writew_lock() error", name);
  }
  printf("%s: got the lock, byte %lld\n", name, (long long)offset);
}

int main(void) {
  int fd;
  pid_t pid;

  /*
   * Create a file and write two bytes to it.
   */
  if ((fd = creat("temp.lock", FILE_MODE)) < 0) {
    err_sys("creat() error");
  }
  if (write(fd, "ab", 2) != 2) {
    err_sys("write() error");
  }

  TELL_WAIT();
  if ((pid = fork()) < 0) {
    err_sys("fork() error");
  } else if (pid == 0) { /* child */
    lock_a_byte("child", fd, 0);
    TELL_PARENT(getpid());
    WAIT_PARENT();
    lock_a_byte("child", fd, 1);
  } else { /* parent */
    lock_a_byte("parent", fd, 1);
    TELL_CHILD(pid);
    WAIT_CHILD();
    lock_a_byte("parent", fd, 0);
  }
  exit(0);
}
