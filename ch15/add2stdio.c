/*
 * This program is an alternative implementation of the add2 program, using
 * standard I/O (rather than the low-level I/O UNIX system calls in add2).
 */
#include "apue.h"

int main(void)
{
  int int1, int2;
  char line[MAXLINE];

  /*
   * The default standard I/O buffering will create a deadlock between the
   * parent and child coprocess.  When this program gets invoked, the first
   * fgets() on the standard input causes the standard I/O library to allocate
   * a buffer and choose the type of buffering.  Since the standard input is a
   * pipe, the standard I/O library defaults to fully buffered.  The same thing
   * happens with the standard output.  While this program is blocked reading
   * from its standard input, the parent process is blocked reading from the
   * pipe, creating a deadlock.
   */

  /*
   * Set stdin to line buffer mode.  This makes fgets() return when a line is
   * available
   */
  if (setvbuf(stdin, NULL, _IOLBF, 0) != 0) {
    err_sys("setvbuf() error");
  }
  /*
   * Set stdout to line buffer mode.  This makes printf() do an fflush() when a
   * newline is output
   */
  if (setvbuf(stdout, NULL, _IOLBF, 0) != 0) {
    err_sys("setvbuf() error");
  }

  while (fgets(line, MAXLINE, stdin) != NULL) {
    if (sscanf(line, "%d%d", &int1, &int2) == 2) {
      if (printf("%d\n", int1 + int2) == EOF) {
        err_sys("printf() error");
      }
    } else {
      if (printf("Invalid args\n") == EOF) {
        err_sys("printf() error");
      }
    }
  }
  exit(0);
}
