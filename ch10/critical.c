/*
 * This program blocks SIGQUIT, saving its current signal mask (to restore
 * later), and then goes to sleep for 5 seconds.  Any occurrence of the quit
 * signal during this period is blocked and won't be delivered until the signal
 * is unblocked.  At the end of the 5 second sleep, a check of whether the
 * signal is pending is made and the signal is unblocked.
 */
#include "apue.h"

static void sig_quit(int);

int main(void) {
  sigset_t newmask, oldmask, pendmask;

  if (signal(SIGQUIT, sig_quit) == SIG_ERR) {
    err_sys("can't catch SIGQUIT");
  }

  /*
   * Block SIGQUIT and save current (old) signal mask.
   */
  sigemptyset(&newmask);
  sigaddset(&newmask, SIGQUIT);
  /* Add SIGQUIT to the signal mask of the process & store previous mask */
  if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
    err_sys("SIG_BLOCK error");
  }

  sleep(5); /* SIGQUIT here will remain pending */

  /*
   * Get the set of signals that are blocked from delivery and currently
   * pending for this process.
   */
  if (sigpending(&pendmask) < 0) {
    err_sys("sigpending error");
  }
  if (sigismember(&pendmask, SIGQUIT)) {
    printf("\nSIGQUIT pending\n");
  }

  /*
   * Restore signal mask; unblocks SIGQUIT.
   *
   * NOTE: Alternatively could SIG_UNBLOCK the SIGQUIT signal.  However, if you
   * need to block a signal in a function that can be called by others, you
   * cannot use SIG_UNBLOCK to unblock the signal because it's possible that
   * the caller had specifically blocked this signal before calling the
   * function.
   */
  if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
    err_sys("SIG_SETMASK error");
  }
  printf("SIGQUIT unblocked\n");

  sleep(5); /* SIGQUIT here will terminate with core file */
  exit(0);
}

static void sig_quit(int signo) {
  printf("caught SIGQUIT\n");
  if (signal(SIGQUIT, SIG_DFL) == SIG_ERR) {
    err_sys("can't reset SIGQUIT");
  }
}
