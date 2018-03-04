/*
 * This program demonstrates how to manipulate terminal special characters.  It
 * disables the interrupt character (^C) and sets the EOF character to ^B.  It
 * is best to run this program in a separate terminal and then kill the terminal
 * with ^B, rather than the usual EOF character ^D; this program does not
 * restore the terminal to its initial state prior to modifying the terminal
 * special characters.
 */
#include "apue.h"
#include <termios.h>

int main(void)
{
  struct termios term;
  long vdisable;

  if (isatty(STDIN_FILENO) == 0) {
    err_quit("Standard input is not a terminal device");
  }

  /*
   * POSIX.1 enable terminal special characters to be disabled by setting the
   * value of an entry in the c_cc[] array to the value _POSIX_VDISABLE.  First
   * check this is supported on the platform.
   */
  if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) < 0) {
    err_quit("fpathconf() error or _POSIX_VDISABLE not in effect");
  }

  if (tcgetattr(STDIN_FILENO, &term) < 0) {
    err_sys("tcgetattr() error");
  }

  /* Modify the terminal control characters */
  term.c_cc[VINTR] = vdisable;  /* disable INTR character (^C) */
  term.c_cc[VEOF] = 2;          /* EOF is Control-B */

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0) {
    err_sys("tcsetattr() error");
  }
  exit(0);
}
