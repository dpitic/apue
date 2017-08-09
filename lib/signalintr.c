/*
 * This function demonstrates an implementation of the signal() function that
 * attempts to prevent any interrupted system calls from being restarted.
 */
#include "apue.h"

Sigfunc *signal_intr(int signo, Sigfunc *func) {
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask); /* initialise signal mask; don't block any */
  act.sa_flags = 0;          /* clear signal behaviour modification flag */
#ifdef SA_INTERRUPT
  /*
   * For improved portability, prevent interrupted system calls from being
   * restarted.
   */
  act.sa_flags |= SA_INTERRUPT;
#endif
  if (sigaction(signo, &act, &oact) < 0) {
    return (SIG_ERR);
  }
  return (oact.sa_handler);
}