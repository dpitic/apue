#include "apue.h"

#define BUFFSIZE 512

static void sig_term(int);                /* defined below */
static volatile sig_atomic_t sigcaught;   /* set by signal handler */

/**
 * Copy everything received from the standard input to the PTY master and
 * everything from the PTY master to standard output.
 * @param ptym file descriptor of master pseudo terminal.
 * @param ignoreeof if != 0 terminate when EOF is encountered on stdin; if 0
 * just notify the parent.
 */
void loop(int ptym, int ignoreeof) {
  pid_t child;
  int nread;
  char buf[BUFFSIZE];

  if ((child = fork()) < 0) {
    err_sys("fork() error");
  } else if (child == 0) {    /* child copies stdin to ptym */
    for ( ; ; ) {
      if ((nread = read(STDIN_FILENO, buf, BUFFSIZE)) < 0) {
        err_sys("read() error from stdin");
      } else if (nread == 0) {
        break;    /* EOF on stdin means we're done */
      }
      if (writen(ptym, buf, nread) != nread) {
        err_sys("writen() error to master pty");
      }
    }

    /*
     * Always terminate when encountering an EOF on stdin, but notify the parent
     * only if ignoreeof is 0.
     */
    if (ignoreeof == 0) {
      kill(getpid(), SIGTERM);    /* notify parent */
    }
    exit(0);    /* and terminate; child can't return */
  }

  /*
   * Parent copies ptym to stdout.
   */
  if (signal_intr(SIGTERM, sig_term) == SIG_ERR) {
    err_sys("signal_intr() error for SIGTERM");
  }

  for ( ; ; ) {
    if ((nread = read(ptym, buf, BUFFSIZE)) <= 0) {
      break;    /* signal caught, error, or EOF */
    }
    if (writen(STDOUT_FILENO, buf, nread) != nread) {
      err_sys("writen() error to stdout");
    }
  }

  /*
   * There are three ways to get here: sig_term() below caught the SIGTERM from
   * the child, we read an EOF on the pty master (which means we have to signal
   * the child to stop), or an error.
   */
  if (sigcaught == 0) {   /* tell child if it didn't send us the signal */
    kill(child, SIGTERM);
  }

  /*
   * Parent returns to caller.
   */
}

/**
 * The child sends us SIGTERM when it gets EOF on the pty slave or when read()
 * fails.  We probably interrupted the read() of ptym.
 * @param signo signal caught; not used.
 */
static void sig_term(int signo) {
  sigcaught = 1;    /* just set flag and return */
}
