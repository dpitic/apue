/*
 * This function demonstrates the use of file and record locking to ensure that
 * only one copy of a daemon is running.  If each daemon creates a file with a
 * fixed name and places a write lock on the entire file, only one such write 
 * lock will be allowed to be created.  Successive attempts to create write
 * locks will fail, serving as an indication to successive copies of the daemon
 * that another instance is already running.
 */
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define LOCKFILE "/var/run/daemon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

extern int lockfile(int);

/*
 * This function tries create a file and write its process ID in the file to
 * enable administrators to identify the process easily.  If the file is already
 * locked, the lockfile() function will fail with errno set to EACCES or
 * EAGAIN, and the function will return 1, indicating that the daemon is already
 * running.  Otherwise the file is truncated and the process ID is written to
 * it, and the function returns 0.
 */
int already_running(void) {
  int fd;
  char buf[16];

  fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
  if (fd < 0) {
    syslog(LOG_ERR, "Can't open %s: %s", LOCKFILE, strerror(errno));
    exit(1);
  }

  if (lockfile(fd) < 0) {
    if (errno == EACCES || errno == EAGAIN) {
      close(fd);
      return(1);
    }
    syslog(LOG_ERR, "Can't lock %s: %s", LOCKFILE, strerror(errno));
    exit(1);
  }

  /* Truncate the file to remove any possible previous PID */
  ftruncate(fd, 0);
  sprintf(buf, "%ld", (long)getpid());
  write(fd, buf, strlen(buf)+1);
  return(0);
}
