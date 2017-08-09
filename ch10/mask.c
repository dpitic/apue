/*
 * This program demonstrates how the signal mask that is installed by the
 * system when a signal handler is invoked automatically includes the signal
 * being caught.  It illustrates the use of the sigsetjmp() and siglongjmp()
 * functions.
 */
#include "apue.h"
#include <setjmp.h>
#include <time.h>

static void sig_usr1(int);
static void sig_alrm(int);
static sigjmp_buf jmpbuf;
/*
 * This type of variable can be written without being interrupted.  This means
 * that a variable of this type should not extend across page boundaries on a
 * system with virtual memory and can be accessed with a single machine
 * instruction.  The volatile qualifier for this data type is included since
 * the variable is being accessed by two different threads: the main function
 * and the asynchronously executing signal handler.
 */
static volatile sig_atomic_t canjump;

int main(void) {
  if (signal(SIGUSR1, sig_usr1) == SIG_ERR) {
    err_sys("signal(SIGUSR1) error");
  }
  if (signal(SIGALRM, sig_alrm) == SIG_ERR) {
    err_sys("signal(SIGALRM) error");
  }

  pr_mask("starting main: ");

  if (sigsetjmp(jmpbuf, 1)) {
    pr_mask("ending main: ");
    exit(0);
  }

  /*
   * This technique should be used whenever siglongjmp() is called from a signal
   * handler.  The variable canjump is set to a nonzero value only after the
   * call to sigsetjmp().  This variable is examined in the signal handler, and
   * siglongjmp() is called only if the flag canjump is nonzero.  This technique
   * provides protection against the signal handler being called at some earlier
   * or later time, when the jump buffer hasn't been initialised by sigsetjmp().
   */
  canjump = 1;  /* now sigsetjmp() is OK */

  for ( ; ; ) {
    pause();
  }
}

static void sig_usr1(int signo) {
  time_t starttime;

  if (canjump == 0) {
    return;   /* unexpected signal, ignore */
  }

  pr_mask("starting sig_usr1: ");

  alarm(3);   /* SIGALRM in 3 seconds */
  starttime = time(NULL);
  for ( ; ; ) {
    if (time(NULL) > starttime + 5) {
      break;
    }
  }

  pr_mask("finishing sig_usr1: ");

  canjump = 0;
  siglongjmp(jmpbuf, 1);    /* jump back to main, don't return */
}

static void sig_alrm(int signo) {
  pr_mask("in sig_alrm: ");
}
