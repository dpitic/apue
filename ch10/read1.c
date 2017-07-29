/*
 * A common use for alarm(), in addition to implementing the sleep() function,
 * is to put an upper time limit on operations that can block.  For example, if
 * we have a read() operation on a device that can block, we might want the
 * read() to time out after some amount of time.  This program does this,
 * reading one line from stdin and writing it ot stdout.
 */
#include "apue.h"

static void sig_alrm(int);

int main(void) {
  int n;
  char line[MAXLINE];

  if (signal(SIGALRM, sig_alrm) == SIG_ERR) {
    err_sys("signal(SIGALRM) error");
  }

  alarm(10);
  /*
   * There is a race condition between alarm() and read().  If the kernel blocks
   * the process between these two function calls for longer than the alarm
   * period, the read() could block forever.  Most operations of this type use
   * a long alamr period, such as a minute or more, making this unlikely;
   * nevertheless, it is a race condition.
   */
  if ((n = read(STDIN_FILENO, line, MAXLINE)) < 0) {
    err_sys("read error");
  }
  alarm(0);

  write(STDOUT_FILENO, line, n);
  exit(0);
}

static void sig_alrm(int signo) {
  /* nothing to do; just return to interrupt the read() */
}
