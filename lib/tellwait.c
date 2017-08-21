/*
 * Utility routines that use signaling to avoid race conditions between parent
 * and child processes.
 */
#include "apue.h"

static volatile sig_atomic_t sigflag; /* set nonzero by signal handler */
static sigset_t newmask, oldmask, zeromask;

/* One signal handler for SIGURS1 and SIGUSR2 */
static void sig_usr(int signo) { sigflag = 1; }

/*
 * Set things up for TELL_xxx() and WAIT_xxx().
 */
void TELL_WAIT(void) {
  if (signal(SIGUSR1, sig_usr) == SIG_ERR) {
    err_sys("signal(SIGUSR1) error");
  }
  if (signal(SIGUSR2, sig_usr) == SIG_ERR) {
    err_sys("signal(SIGUSR2) error");
  }
  sigemptyset(&zeromask);
  sigemptyset(&newmask);
  sigaddset(&newmask, SIGUSR1);
  sigaddset(&newmask, SIGUSR2);

  /* Block SIGUSR1 and SIGUSR2, and save current signal mask */
  if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
    err_sys("SIG_BLOCK error");
  }
}

/*
 * Tell parent we're done.
 */
void TELL_PARENT(pid_t pid) { kill(pid, SIGUSR2); }

void WAIT_PARENT(void) {
  while (sigflag == 0) {
    sigsuspend(&zeromask); /* and wait for parent */
  }
  sigflag = 0;

  /* Reset signal mask to original value */
  if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
    err_sys("SIG_SETMASK error");
  }
}

/*
 * Tell child we're done.
 */
void TELL_CHILD(pid_t pid) { kill(pid, SIGUSR1); }

void WAIT_CHILD(void) {
  while (sigflag == 0) {
    sigsuspend(&zeromask); /* and wait for child */
  }

  /* Reset signal mask to original value */
  if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
    err_sys("SIG_SETMASK error");
  }
}
