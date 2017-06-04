/*
 * This program outputs two strings: one from the child and one from the parent.
 * The program contains a race condition because the output depends on the
 * order in which the processes are run by the kernel and the length of time for
 * which each process runs.  In order to deal with the race condition, this
 * program uses signaling techniques to inform the parent & child processes and
 * prevent intermixing of output from the two processes.
 */
#include "apue.h"

static void charatatime(char *);

int main(void) {
  pid_t pid;

  TELL_WAIT();    /* set things up for TELL_xxx() and WAIT_xxx() */

  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) { /* child */
    WAIT_PARENT();      /* parent goes first */
    charatatime("output from child\n");
  } else {              /* parent */
    charatatime("output from parent\n");
    TELL_CHILD(pid);    /* tell child we're done */
  }
  exit(0);
}

static void charatatime(char *str) {
  char *ptr;
  int c;

  setbuf(stdout, NULL);   /* set unbuffered */
  for (ptr = str; (c = *ptr++) != 0; ) {
    putc(c, stdout);
  }
}