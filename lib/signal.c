/*
 * Reliable, cross-platform implementation of signal() using POSIX sigaction().
 */
#include "apue.h"

Sigfunc *signal(int signo, Sigfunc *func) {
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);  /* initialise mask; don't block any signals */
  act.sa_flags = 0;           /* clear signal behaviour modification flag */
  if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
    /* 
     * For older systems such as SunOS which restart interrupted system calls
     * by default.  Specifying this flag causes system calls to be interrupted.
     * Linux defines this flag for compatibility with applications that use it,
     * but by default does not restart system calls when the signal handler is
     * installed with sigaction().  The SUS specifies that the sigaction() 
     * function not restart interrupted system calls unless SA_RESTART flag is
     * specified.
     *
     * System calls interrupted by this signal are not automatically restarted.
     * This is the XSI default for sigaction().
     */
    act.sa_flags |= SA_INTERRUPT;
#endif
  } else {
    /* 
     * System calls interrupted by this signal are automatically restarted for
     * all signals other than SIGALRM.  By not restarting SIGALRM allows a
     * timeout for I/O operations to be set.
     */
    act.sa_flags |= SA_RESTART;
  }
  if (sigaction(signo, &act, &oact) < 0) {
    return (SIG_ERR);
  }
  return (oact.sa_handler);
}