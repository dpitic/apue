/*
 * Pseudo terminal handling functions.
 */
#include "apue.h"
#include <errno.h>
#include <fcntl.h>

#if defined(SOLARIS)
#include <stropts.h>
#endif

/**
 * Open the next available PTY master and initialise the corresponding PTY slave
 * device.  The caller must allocate an array to hold the name of the slave;
 * if the call succeeds, the name of the corresponding slave is returned through
 * pts_name pointer.  This name is then passed to ptys_open(), which opens the
 * corresponding slave device.
 * @param pts_name pointer to buffer containing the slave PTY name.
 * @param pts_namez length of the buffer in bytes for slave PTY name.
 * @return file descriptor of PTY master on success; -1 on error.
 */
int ptym_open(char *pts_name, int pts_namesz) {
  char *ptr; /* slave PTY name */
  int fdm, err;

  if ((fdm = posix_openpt(O_RDWR)) < 0) {
    return (-1);
  }
  if (grantpt(fdm) < 0) { /* grant access to slave */
    goto errout;
  }
  if (unlockpt(fdm) < 0) { /* clear slave's lock flag */
    goto errout;
  }
  if ((ptr = ptsname(fdm)) == NULL) { /* get slave's name */
    goto errout;
  }

  /*
   * Return name of slave.  Null terminate to handle case where
   * strlen(ptr) > pts_namesz.
   */
  strncpy(pts_name, ptr, pts_namesz);
  pts_name[pts_namesz - 1] = '\0';
  return (fdm); /* return fd of master PTY */
errout:
  err = errno;
  close(fdm);
  errno = err;
  return (-1);
}

/**
 * Open slave PTY device corresponding to master PTY name pts_name.
 * @param pointer to name of master PTY.
 * @return file descriptor of PTY slave on success; -1 on error.
 */
int ptys_open(char *pts_name) {
  int fds;
#if defined(SOLARIS)
  int err, setup;
#endif

  if ((fds = open(pts_name, O_RDWR)) < 0) {
    return (-1);
  }

#if defined(SOLARIS)
  /*
   * Check if stream is already set up by autopush facility.
   */
  if ((setup = ioctl(fds, I_FIND, "ldterm")) < 0) {
    goto errout;
  }

  if (setup == 0) {
    if (ioctl(fds, I_PUSH, "ptem") < 0) {
      goto errout;
    }
    if (ioctl(fds, I_PUSH, "ldterm") < 0) {
      goto errout;
    }
    if (ioctl(fds, I_PUSH, "ttcompat") < 0) {
    errout:
      err = errno;
      close(fds);
      errno = err;
      return (-1);
    }
  }
#endif
  return (fds);
}
