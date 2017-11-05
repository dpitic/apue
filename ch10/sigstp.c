/*
 * This program demonstrates the normal sequence of code used when a program
 * handles job control.  It copies its standard input to its standard output,
 * but comments are given in the signal handler for typical actions performed by
 * a program that manages a screen.
 */
#include "apue.h"

#define BUFFSIZE 1024

/* Signal handler for SIGTSTP */
static void sig_tstp(int signo) {
  sigset_t mask;

  /* 
   * Perform any terminal related processing such as move cursor to lower left 
   * corner, reset tty mode ... 
   */

  signal(SIGTSTP, SIG_DFL); /* reset disposition to default */
  kill(getpid(), SIGTSTP);  /* and send the signal to ourself */

  /*
   * Unblock SIGTSTP, since it's blocked while we're handling it.
   * We won't return from sigprocmask() until we're continued.
   */
  sigemptyset(&mask);
  sigaddset(&mask, SIGTSTP);
  sigprocmask(SIG_UNBLOCK, &mask, NULL);

  signal(SIGTSTP, sig_tstp); /* reestablish signal handler */

  /* ... reset tty mode, redraw screen ... */
}

int main(void) {
  int n;
  char buf[BUFFSIZE];

  /*
   * Only catch SIGTSTP if we're running with a job-control shell.  The init
   * process sets the disposotion of the three job control signals (SIGTSTP,
   * SIGTTIN, SIGTTOU) to SIG_IGN, which is then inherited by all login shells.
   * Only a job control shell should reset the disposition of these threee
   * signals to SIG_DFL.
   */
  if (signal(SIGTSTP, SIG_IGN) == SIG_DFL) {
    signal(SIGTSTP, sig_tstp);
  }

  while ((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0) {
    if (write(STDOUT_FILENO, buf, n) != n) {
      err_sys("write() error");
    }
  }

  if (n < 0) {
    err_sys("read() error");
  }

  exit(0);
}