/*
 * Implementation of the POSIX.1 sleep() function.  This function handles
 * signals reliably, avoiding the race conditions of earlier implementations.
 * It still doesn't handle any interactions with previously set alarms, as
 * these interactions are explicitly undefined by POSIX.1.
 */
#include "apue.h"

static void sig_alrm(int signo) {
  /* Nothing to do, just returning wakes up sigsuspend() */
}

unsigned int sleep(unsigned int seconds) {
  struct sigaction newact, oldact;
  sigset_t newmask, oldmask, suspmask;
  unsigned int unslept;

  /* Set the handler, save previous information */
  newact.sa_handler = sig_alrm;
  sigemptyset(&newact.sa_mask);
  newact.sa_flags = 0;
  sigaction(SIGALRM, &newact, &oldact);

  /* Block SIGALRM and save current signal mask */
  sigemptyset(&newmask);
  sigaddset(&newmask, SIGALRM);
  sigprocmask(SIG_BLOCK, &newmask, &oldmask);

  alarm(seconds);
  suspmask = oldmask;

  /* Make sure SIGALRM isn't blocked */
  sigdelset(&suspmask, SIGALRM);

  /* Wait for any signal to be caught */
  sigsuspend(&suspmask);

  /* Some signal has been caught, SIGALRM is now blocked */

  unslept = alarm(0);

  /* Reset previous action */
  sigaction(SIGALRM, &oldact, NULL);

  /* Reset signal mask, which unblocks SIGALRM */
  sigprocmask(SIG_SETMASK, &oldmask, NULL);
  return (unslept);
}