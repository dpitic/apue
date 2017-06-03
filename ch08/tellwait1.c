/*
 * This program outputs two strings: one from the child and one from the parent.
 * The program contains a race condition because the output depends on the
 * order in which the processes are run by the kernel and the length of time for
 * which each process runs.
 */
#include "apue.h"

static void charatatime(char *);

int main(void) {
  pid_t pid;

  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) { /* child */
    charatatime("output from child\n");
  } else {              /* parent */
    charatatime("output from parent\n");
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