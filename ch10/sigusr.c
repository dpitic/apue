/*
 * Simple signal handler that catches either of the two user defined signals
 * and prints the signal number.  This program runs an infinite loop that waits
 * for a signal to be sent to the process.  Best to run this program in the
 * background and send signals to it using kill(1).
 */
#include "apue.h"

static void sig_usr(int); /* one handler for both user signals */

int main(void) {
  if (signal(SIGUSR1, sig_usr) == SIG_ERR) {
    err_sys("can't catch SIGUSR1");
  }
  if (signal(SIGUSR2, sig_usr) == SIG_ERR) {
    err_sys("can't catch SIGUSR2");
  }
  /* Set up infinite loop waiting to receive a signal */
  for (;;) {
    /* Suspend the calling process until a signal is received */
    pause();
  }
}

/*
 * Signal handler for signal number signo.  Catches the two user signals and
 * and prints the signal number, otherwise prints the signal number, dumps core
 * and terminates for all other signals.
 */
static void sig_usr(int signo) {
  if (signo == SIGUSR1) {
    printf("received SIGUSR1\n");
  } else if (signo == SIGUSR2) {
    printf("received SIGUSR2\n");
  } else {
    err_dump("received signal %d\n", signo);
  }
}