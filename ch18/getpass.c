/*
 * This is a demonstration of an implementation of the getpass() function.
 */
#include <signal.h>
#include <stdio.h>
#include <termios.h>

#define MAX_PASS_LEN 8 /* max # chars for user to enter */

/**
 * Read a password of some type from the user at a terminal with echoing off.
 * @param prompt string to display as the user prompt.
 * @return pointer to string containing password entered by the user.
 */
char *getpass(const char *prompt) {
  static char buf[MAX_PASS_LEN + 1]; /* null byte at end */
  char *ptr;
  sigset_t sig, osig;
  struct termios ts, ots;
  FILE *fp;
  int c;

  /* Open controlling terminal for the process */
  if ((fp = fopen(ctermid(NULL), "r+")) == NULL) {
    return (NULL);
  }
  setbuf(fp, NULL);

  sigemptyset(&sig);
  /*
   * Block signals to prevent terminating or suspending the program while
   * echoing is disabled.  If these signals are generated while reading the
   * password, they are held until the function returns.  SIGQUIT is not
   * blocked, so entering the QUIT character aborts the program and probably
   * leaves the terminal with echoing disabled.
   */
  sigaddset(&sig, SIGINT);             /* block SIGINT */
  sigaddset(&sig, SIGTSTP);            /* block SIGTSTP */
  sigprocmask(SIG_BLOCK, &sig, &osig); /* and save mask */

  tcgetattr(fileno(fp), &ts); /* save tty state */
  ots = ts;                   /* structure copy */
  /*
   * Turn off echo:
   *   ECHO   = enable echo
   *   ECHOE  = visually erase chars
   *   ECHOK  = echo kill
   *   ECHONL = echo newline
   */
  ts.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
  tcsetattr(fileno(fp), TCSAFLUSH, &ts);
  fputs(prompt, fp);

  ptr = buf;
  while ((c = getc(fp)) != EOF && c != '\n') {
    if (ptr < &buf[MAX_PASS_LEN]) {
      *ptr++ = c;
    }
  }
  *ptr = 0;       /* null terminate */
  putc('\n', fp); /* echo a newline */

  tcsetattr(fileno(fp), TCSAFLUSH, &ots); /* restore TTY state */
  sigprocmask(SIG_SETMASK, &osig, NULL);  /* restore mask */
  fclose(fp);                             /* finished with /dev/tty */
  return (buf);
}