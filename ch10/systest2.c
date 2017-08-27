/*
 * This program catches both the SIGINT and SIGCHLD signals.  It invokes the
 * ed(1) editor using the system() function.  The ed editor is an interactive
 * program that catches the interrupt and quit signals.  If we invoke ed from
 * the shell and type the interrupt character, it catches the interrupt signal
 * and prints the question mark.  It also sets the disposition of the quit
 * signal so that it is ignored.  When the editor terminates, the system sends
 * the SIGCHLD signal to the parent (this program).  This program catches it and
 * returns from the signal handler.  The delivery of this signal in the parent
 * should be blocked while the system function is executing, as specified by
 * POSIX.1.  Otherwise when the child created by system() terminates, it would
 * fool the caller of the system into thinking that one of its own children
 * terminated.  The caller would then use one of the wait() functions to get the
 * termination status of the children, thereby preventing a system() function
 * from being able to obtain the child's termination status for its return
 * value.
 */
#include "apue.h"

static void sig_int(int signo) { printf("caught SIGINT\n"); }

static void sig_chld(int signo) { printf("caught SIGCHLD\n"); }

int main(void) {
  if (signal(SIGINT, sig_int) == SIG_ERR) {
    err_sys("signal(SIGINT) error");
  }
  if (signal(SIGCHLD, sig_chld) == SIG_ERR) {
    err_sys("signal(SIGCHLD) error");
  }
  if (system("/bin/ed") < 0) {
    err_sys("system() error");
  }
  exit(0);
}
