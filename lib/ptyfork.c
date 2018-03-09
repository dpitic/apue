/*
 * Library PTY handling function.
 */

#include "apue.h"
#include <termios.h>

/**
 * This function combines the opening of the master and the slave with a call to
 * fork(), establishing the child as a session leader with a controlling
 * terminal.
 * @param ptrfdm returned pointer to the file descriptor of the PTY master.
 * @param slave_name if non-null, the name of the slave device.  The caller is
 * responsible for allocating the storage pointed to by this argument.
 * @param slave_namesz size of slave_name buffer.
 * @param slave_termios if non-null, the system uses the referenced struct to
 * initialise the terminal line discipline of the slave.  If this pointer is
 * null, the system sets the slave's termios struct to an implementation
 * defined initial state.
 * @param slave_winsize if non-null, the referenced structure initialises the
 * slave's window size.  If this pointer is null, the winsize struct is normally
 * initialised to 0.
 * @return 0 in child; process ID of child in parent; -1 on error.
 */
pid_t pty_fork(int *ptrfdm, char *slave_name, int slave_namesz,
               const struct termios *slave_termios,
               const struct winsize *slave_winsize) {
  int fdm, fds;
  pid_t pid;
  char pts_name[20];

  /* Open master pseudo terminal */
  if ((fdm = ptym_open(pts_name, sizeof(pts_name))) < 0) {
    err_sys("Can't open master pty: %s, error %d", pts_name, fdm);
  }

  if (slave_name != NULL) {
    /*
     * Return name of slave.  Null terminate to handle case where
     * strlen(pts_name) > slave_namesz.
     */
    strncpy(slave_name, pts_name, slave_namesz);
    slave_name[slave_namesz - 1] = '\0';
  }

  if ((pid = fork()) < 0) {
    return(-1);
  } else if (pid == 0) {  /* child */
    /*
     * The child opens the slave PTY after calling setsid() to establish a new
     * sesison.  When the child calls setsid(), the child is not a process group
     * leader, so the following steps occur:
     *   1. A new session is created with the child as the session leader.
     *   2. A new process group is created for the child.
     *   3. The child loses any association it might have had with its previous
     *      controlling terminal.
     * Under Linux, macOS and Solaris, the slave becomes the controlling
     * terminal of this new session when ptys_open() is called.  FreeBSD
     * requires the use of TIOCSCTTY ioctl() command to allocate the controlling
     * terminal.
     */
    if (setsid() < 0) {
      err_sys("setsid() error");
    }

    /*
     * System V acquires controlling terminal on open().
     */
    if ((fds = ptys_open(pts_name)) < 0) {
      err_sys("Can't open slave pty");
    }
    close(fdm);  /* finished with master in child */

#if defined(BSD)
    /*
     * TIOCSCTTY is the BSD way to acquire a controlling terminal.
     */
    if (ioctl(fds, TIOCSCTTY, (char *)0) < 0) {
      err_sys("TIOCSCTTY error");
    }
#endif

    /*
     * Set slave's termios and window size.
     */
    if (slave_termios != NULL) {
      if (tcsetattr(fds, TCSANOW, slave_termios) < 0) {
        err_sys("tcsetattr() error on slave pty");
      }
    }
    if (slave_winsize != NULL) {
      if (ioctl(fds, TIOCSWINSZ, slave_winsize) < 0) {
        err_sys("TIOCSWINSZ error on slave pty");
      }
    }

    /*
     * Slave becomes stdin/stdout/stderr of child.
     */
    if (dup2(fds, STDIN_FILENO) != STDIN_FILENO) {
      err_sys("dup2() error to stdin");
    }
    if (dup2(fds, STDOUT_FILENO) != STDOUT_FILENO) {
      err_sys("dup2() error to stdout");
    }
    if (dup2(fds, STDERR_FILENO) != STDERR_FILENO) {
      err_sys("dup2() error to stderr");
    }
    if (fds != STDIN_FILENO && fds != STDOUT_FILENO && fds != STDERR_FILENO) {
      close(fds);
    }
    return(0);      /* child returns 0 just like fork() */
  } else {          /* parent */
    *ptrfdm = fdm;  /* return fd of master PTY */
    return(pid);    /* parent returns pid of child */
  }
}
