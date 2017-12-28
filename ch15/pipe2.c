/*
 * This program displays some output one page at a time by invoking a pager
 * program.  It pipes the output directly to the pager by creating a pipe and
 * forking a child process and setting up the child's standard input to be the
 * read end of the pipe, and exec() the users' program.
 */
#include "apue.h"
#include <sys/wait.h>

#define DEF_PAGER "/bin/more"     /* default pager program */

int main(int argc, char *argv[argc])
{
  int n;
  int fd[2]; /* pipe read & write file descriptors */
  pid_t pid;
  char *pager, *argv0;
  char line[MAXLINE];
  FILE *fp;

  if (argc != 2) {
    err_quit("Usage: %s <pathname>", argv[0]);
  }

  if ((fp = fopen(argv[1], "r")) == NULL) {
    err_sys("Can't open %s", argv[1]);
  }
  if (pipe(fd) < 0) {
    err_sys("pipe() error");
  }

  if ((pid = fork()) < 0) {
    err_sys("fork() error");
  } else if (pid > 0) { /* parent; writer */
    close(fd[0]);       /* close read end of pipe for writer */

    /* Parent copies argv[1] to pipe */
    while (fgets(line, MAXLINE, fp) != NULL) {
      n = strlen(line);
      if (write(fd[1], line, n) != n) {
        err_sys("write() error to pipe");
      }
    }

    if (ferror(fp)) {
      err_sys("fgets() error");
    }

    close(fd[1]);       /* finished writing; close write end of pipe */

    if (waitpid(pid, NULL, 0) < 0) {
      err_sys("waitpid() error");
    }
    exit(0);
  } else {              /* child; reader */
    close(fd[1]);       /* close write end of pipe for reader */
    if (fd[0] != STDIN_FILENO) {
      if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO) {
        err_sys("dup2() error to stdin");
      }
      close(fd[0]);     /* not required after dup2() */
    }

    /* Get arguments for execl() */
    if ((pager = getenv("PAGER")) == NULL) { /* try environment pager */
      pager = DEF_PAGER; /* otherwise use default pager */
    }
    if ((argv0 = strrchr(pager, '/')) != NULL) {
      argv0++;          /* step past rightmost slash */
    } else {
      argv0 = pager;    /* no slash in pager */
    }

    if (execl(pager, argv0, (char *)0) < 0) {
      err_sys("execl() error for %s", pager);
    }
  }
  exit(0);
}
