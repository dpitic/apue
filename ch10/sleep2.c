/*
 * This program demonstrates a partial SVR2 iimplementation of sleep(), using
 * setjmp and longjmp to avoid the race condition.  This simple implementation
 * also doesn't correct the two other issues described in sleep1().  There is
 * another subtle problem with the sleep2() function that involves its
 * interaction with other signals.  If the SIGALRM interrupts some other
 * signal handler, then when we call longjmp(), we abort the other signal
 * handler.
 */
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

static jmp_buf env_alrm;

static void sig_alrm(int signo) { longjmp(env_alrm, 1); }

unsigned int sleep2(unsigned int seconds) {
  if (signal(SIGALRM, sig_alrm) == SIG_ERR) {
    return (seconds);
  }
  if (setjmp(env_alrm) == 0) {
    alarm(seconds); /* start the timer */
    pause();        /* next caught signal wakes us up */
  }
  return (alarm(0)); /* turn off timer, return unslept time */
}
