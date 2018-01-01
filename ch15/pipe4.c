/*
 * This program invokes the add2 coprocess after reading two numbers from
 * standard input.  The value from the coprocess is written to its standard
 * output.
 */
#include "apue.h"

/*
 * Select which child coprocess to run, add2 or add2stdio;  By default, use
 * add2.  Comment out this define to use add2stdio.
 */
#define ADD2

#if defined(ADD2)
#define PATH "./add2"
#define ARG "add2"
#else 
#define PATH "./add2stdio"
#define ARG "add2stdio"
#endif /* ADD2 */

static void sig_pipe(int); /* our signal handler */

int main(void)
{
  int n;
  int fd1[2]; /* pipe 1 used by parent to write to child stdin */
  int fd2[2]; /* pipe 2 used by parent to read from child stdout */
  pid_t pid;
  char line[MAXLINE];

  if (signal(SIGPIPE, sig_pipe) == SIG_ERR) {
    err_sys("signal() error");
  }

  /*
   * Create the two pipes to communicate with the coprocess.  fd1 pipe is used 
   * by the parent to write to the child stdin.  fd2 pipe is used by the parent
   * to read from the child stdout.
   * =========================================
   * | Parent |---pipe---| Child (coprocess) |
   * -----------------------------------------
   * | fd1[1] |--pipe 1->| stdin             |
   * | fd2[0] |<-pipe 2--| stdout            |
   * =========================================
   */
  if (pipe(fd1) < 0 || pipe(fd2) < 0) {
    err_sys("pipe() error");
  }

  if ((pid = fork()) < 0) {
    err_sys("fork() error");
  } else if (pid > 0) { /* parent */
    close(fd1[0]); /* pipe 1 parent read descriptor not required */
    close(fd2[1]); /* pipe 2 parent write descriptor not required */

    while (fgets(line, MAXLINE, stdin) != NULL) {
      n = strlen(line);
      if (write(fd1[1], line, n) != n) {
        err_sys("write() error to pipe");
      }
      if ((n = read(fd2[0], line, MAXLINE)) < 0) {
        err_sys("read() error from pipe");
      }
      if (n == 0) {
        err_msg("child closed pipe");
        break;
      }
      line[n] = 0; /* null terminate */
      if (fputs(line, stdout) == EOF) {
        err_sys("fputs() error");
      }
    }

    if (ferror(stdin)) {
      err_sys("fgets() error on stdin");
    }
    exit(0);
  } else {         /* child */
    close(fd1[1]); /* pipe 1 child write descriptor not required */
    close(fd2[0]); /* pipe 2 child read descriptor not required */
    if (fd1[0] != STDIN_FILENO) {
      if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO) {
        err_sys("dup2() error to stdin");
      }
      close(fd1[0]);
    }

    if (fd2[1] != STDOUT_FILENO) {
      if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO) {
        err_sys("dup2() error to stdout");
      }
      close(fd2[1]);
    }

    /* Child coprocess execute add2 program */
    if (execl(PATH, ARG, (char *)0) < 0) {
      err_sys("execl() error");
    }
  }
  exit(0);
}

static void sig_pipe(int signo) {
  printf("SIGPIPE caught\n");
  exit(1);
}
