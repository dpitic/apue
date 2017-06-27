/*
 * This program demonstrates orphaned process groups.  It implements a process
 * that forks a child that stops and the parent exits.
 */
#include "apue.h"
#include <errno.h>

/* Signal handler for SIGHUP */
static void sig_hup(int signo) {
  printf("SIGHUP received, pid = %ld\n", (long)getpid());
}

static void pr_ids(char *name) {
  printf("%s: pid = %ld, ppid = %ld, prgrp = %ld, tpgrp = %ld\n", name,
         (long)getpid(), (long)getppid(), (long)getpgrp(),
         (long)tcgetpgrp(STDIN_FILENO));
  fflush(stdout);
}

int main(void) {
  char c;
  pid_t pid;

  pr_ids("parent");
  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid > 0) { /* parent */
    sleep(5);           /* sleep to let child stop itself */
  } else {              /* child */
    pr_ids("child");
    signal(SIGHUP, sig_hup); /* register signal handler */
    kill(getpid(), SIGTSTP); /* stop child */
    pr_ids("child");         /* prints only if child continued */
    if (read(STDIN_FILENO, &c, 1) != 1) {
      printf("read error %d on controlling TTY\n", errno);
    }
  }
  exit(0);
}