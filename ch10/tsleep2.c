/*
 * This program demonstrates the use of the sleep2() function.  It demonstrates
 * the subtle problem in that function's implementation that involves the
 * interaction with other signals.  The loop in the SIGINT handler was written
 * so that it executes for longer than the argument to sleep2().
 */
#include "apue.h"

unsigned int sleep2(unsigned int);
static void sig_int(int);

int main(void) {
  unsigned int unslept;

  if (signal(SIGINT, sig_int) == SIG_ERR) {
    err_sys("signal(SIGINT) error");
  }
  unslept = sleep2(5);
  printf("sleep2 returned: %u\n", unslept);
  exit(0);
}

static void sig_int(int signo) {
  int i, j;
  volatile int k;

  /*
   * Tune these loops to run for more than 5 seconds on whatever system this
   * test program is run on.
   */
  k = 0;
  printf("\nsig_int starting\n");
  for (i = 0; i < 3000000; i++) {
    for (j = 0; j < 40000; j++) {
      k += i * j;
    }
  }
  printf("sig_int finished\n");
}
