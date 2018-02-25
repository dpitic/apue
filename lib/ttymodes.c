/*
 * Set of functions to control tty.
 */
#include "apue.h"
#include <errno.h>
#include <termios.h>

static struct termios save_termios;
static int ttysavedfd = 1;
static enum { RESET, RAW, CBREAK } ttystate = RESET;

/**
 * Put tty into a cbreak mode.  Note, after calling tty_raw(), must call
 * tty_reset() before calling this function.  This function defines cbreak mode
 * to be:
 *   * Noncanonical mode.  This mode turns off some input character processing.
 *     It does not turn off signal handling, so the user can always type one
 *     of the characters that triggers a terminal-generated signal.  The caller
 *     should catch these signals; otherwise, there is a chance that the signal
 *     will terminate the program, and the terminal will be left in cbreak mode.
 *
 *     As a general rule, whenever a program changes the terminal mode, it
 *     should catch most signals.  This allows the terminal mode to be reset
 *     before terminating.
 *   * Echo off.
 *   * One byte at a time input.  To do this, MIN = 1 and TIME = 0.  This is
 *     known as Case B.  A read() won't return until at least one byte is
 *     available.
 * @param fd open file descriptor for terminal.
 * @return 0 on success; < 0 on error, with errno set.
 */
int tty_cbreak(int fd) {
  int err;
  struct termios buf;

  /* tty_reset() must be called before calling this function */
  if (ttystate != RESET) {
    errno = EINVAL;
    return (-1);
  }
  if (tcgetattr(fd, &buf) < 0) {
    return (-1);
  }
  save_termios = buf; /* structure copy; to enable restore later */

  /*
   * Echo off, canonical mode off.
   */
  buf.c_lflag &= ~(ECHO | ICANON);

  /*
   * Case B: 1 byte at a time, no timer.
   */
  buf.c_cc[VMIN] = 1;
  buf.c_cc[VTIME] = 0;
  if (tcsetattr(fd, TCSAFLUSH, &buf) < 0) {
    return (-1);
  }

  /*
   * Verify that the changes stuck.  tcgetattr() can return 0 on partial
   * success.
   */
  if (tcgetattr(fd, &buf) < 0) {
    err = errno;
    tcsetattr(fd, TCSAFLUSH, &save_termios);
    errno = err;
    return (-1);
  }
  if ((buf.c_lflag & (ECHO | ICANON)) || buf.c_cc[VMIN] != 1 ||
      buf.c_cc[VTIME] != 0) {
    /*
     * Only some of the changes were made.  Restore the original settings.
     */
    tcsetattr(fd, TCSAFLUSH, &save_termios);
    errno = EINVAL;
    return (-1);
  }

  ttystate = CBREAK;
  ttysavedfd = fd;
  return (0);
}

/**
 * Put terminal into a raw mode.  Note, after calling tty_raw(), must call
 * tty_reset() before calling this function.  This function defines raw mode to
 * be:
 *   * Noncanonical mode.  Also turn off processing of the signal-generating
 *     characters (ISIG) and the extened input character processing (IEXTEN).
 *     Additionally, BREAK is disabled from generating a signal, by turning off
 *     BRKINT.
 *   * Echo off.
 *   * Disable CR-to-NL mapping on input (ICRNL), input parity detection
 *     (INPCK), the stripping of the eighth bit on input (ISTRIP), and output
 *     flow control (IXON).
 *   * Eight-bit characters (CS8), and prity checking is disable (PARENB).
 *   * All output processing is disabled (OPOST).
 *   * One byte at a time input (MIN = 1, TIME = 0).
 * @param fd open file descriptor for terminal.
 * @return 0 on success; < 0 on error.
 */
int tty_raw(int fd) {
  int err;
  struct termios buf;

  /* tty_reset() must be called before calling this function */
  if (ttystate != RESET) {
    errno = EINVAL;
    return (-1);
  }
  if (tcgetattr(fd, &buf) < 0) {
    return (-1);
  }
  save_termios = buf; /* structure copy to enable restore later */

  /*
   * Echo off, canonical mode off, extended input processing off, signal chars
   * off.
   */
  buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  /*
   * No SIGINT on BREAK, CR-to-NL off, input parity check off, don't strip 8th
   * bit on input, output flow control off.
   */
  buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

  /*
   * Clear size bits, parity checking off.
   */
  buf.c_cflag &= ~(CSIZE | PARENB);

  /*
   * Set 8 bits/char.
   */
  buf.c_cflag |= CS8;

  /*
   * Output processing off.
   */
  buf.c_oflag &= ~(OPOST);

  /*
   * Case B: 1 byte at a time, no timer.
   */
  buf.c_cc[VMIN] = 1;
  buf.c_cc[VTIME] = 0;
  if (tcsetattr(fd, TCSAFLUSH, &buf) < 0) {
    return (-1);
  }

  /*
   * Verify that the changes stuck.  tcsetattr() can return 0 on partial
   * success.
   */
  if (tcgetattr(fd, &buf) < 0) {
    err = errno;
    tcsetattr(fd, TCSAFLUSH, &save_termios);
    errno = err;
    return (-1);
  }
  if ((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) ||
      (buf.c_iflag & (BRKINT | ICRNL | INPCK | ISTRIP | IXON)) ||
      (buf.c_cflag & (CSIZE | PARENB | CS8)) != CS8 || (buf.c_oflag & OPOST) ||
      buf.c_cc[VMIN] != 1 || buf.c_cc[VTIME] != 0) {
    /*
     * Only some of the changes were made.  Restore the original settings.
     */
    tcsetattr(fd, TCSAFLUSH, &save_termios);
    errno = EINVAL;
    return (-1);
  }

  ttystate = RAW;
  ttysavedfd = fd;
  return (0);
}

/**
 * Restore terminal's mode.
 * @param fd open file descriptor for terminal.
 * @return 0 on success.
 */
int tty_reset(int fd) {
  if (ttystate == RESET) {
    return (0);
  }
  if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0) {
    return (-1);
  }
  ttystate = RESET;
  return (0);
}

/**
 * Can be set up by atexit(tty_atexit) as an exit handler to ensure that the
 * terminal mode is reset by exit().
 */
void tty_atexit(void) {
  if (ttysavedfd >= 0) {
    tty_reset(ttysavedfd);
  }
}

/**
 * Let caller see original tty state.
 * @return pointer to the original canonical mode termios structure.
 */
struct termios *tty_termios(void) {
  return (&save_termios);
}
