/*
 * This program demonstrates how to reestablish the signal handler.  The signal
 * handler must be reestablished on entry to the signal handler by calling
 * signal() again.  This action is intended to minimise the window of time when
 * the signal is reset back to its default and could get lost.  This program
 * doesn't work on traditional System V platforms.  The output is a continual
 * string of SIGCLD received lines.  Eventually the process runs out of stack
 * space and terminates abnormally.
 */
#include "apue.h"
#include <sys/wait.h>

static void sig_cld(int);

int main() {
  pid_t pid;

  if (signal(SIGCLD, sig_cld) == SIG_ERR) {
    perror("signal error");
  }
  if ((pid = fork()) < 0) {
    perror("fork error");
  } else if (pid == 0) {  /* child */
    sleep(2);
    _exit(0);
  }

  pause();  /* parent */
  exit(0);
}

/* Interrupts pause() */
static void sig_cld(int signo) {
  pid_t pid;
  int status;

  printf("SIGCLD received\n");

  if (signal(SIGCLD, sig_cld) == SIG_ERR) { /* reestablish signal handler */
    perror("signal error");
  }
  if ((pid = wait(&status)) < 0) {  /* fetch child status */
    perror("wait error");
  }
  printf("pid = %d\n", pid);
}
