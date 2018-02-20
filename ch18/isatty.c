/*
 * Sample implementation of the isatty() function.  THis trivial implementation
 * simply tries one of the terminal specific functions, that doesn't change
 * anything if it succeeds, and looks at the return value.
 */
#include <termios.h>

/**
 * Test whether the file descriptor is an open file descriptor referring to
 * a terminal.
 * @param fd file descriptor to test.
 * @return 1 if fd is an open file descriptor referring to a terminal; otherwise
 * 0.
 */
int isatty(int fd) {
  struct termios ts;

  return(tcgetattr(fd, &ts) != -1);  /* true if no error (is a tty) */
}
