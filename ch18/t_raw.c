/*
 * This program tests raw and cbreak tty modes.
 */
#include "apue.h"

/**
 * Signal handler.  Used to reset the terminal and exit the program after.
 * @param signo signal number; not used.
 */
static void sig_catch(int signo) {
  printf("Signal caught\n");
  tty_reset(STDIN_FILENO);
  exit(0);
}

int main(void) {
  int i;
  char c;

  /* Catch signals */
  if (signal(SIGINT, sig_catch) == SIG_ERR) {
    err_sys("signal(SIGINT) error");
  }
  if (signal(SIGQUIT, sig_catch) == SIG_ERR) {
    err_sys("signal(SIGQUIT) error");
  }
  if (signal(SIGTERM, sig_catch) == SIG_ERR) {
    err_sys("signal(SIGTERM) error");
  }

  /* Raw mode */
  if (tty_raw(STDIN_FILENO) < 0) {
    err_sys("tty_raw() error");
  }
  printf("Enter raw mode characters, terminate with DELETE\n");
  while ((i = read(STDIN_FILENO, &c, 1)) == 1) {
    if ((c &= 255) == 0177) { /* 0177 = ASCII DELETE */
      break;
    }
    printf("%o\n", c);
  }
  if (tty_reset(STDIN_FILENO) < 0) {
    err_sys("tty_reset() error");
  }
  if (i <= 0) {
    err_sys("read() error");
  }

  /* cbreak mode */
  if (tty_cbreak(STDIN_FILENO) < 0) {
    err_sys("tty_cbreak() error");
  }
  printf("\nEnter cbreak mode characters, terminate with SIGINT\n");
  while ((i = read(STDIN_FILENO, &c, 1)) == 1) {
    c &= 255;
    printf("%o\n", c);
  }
  if (tty_reset(STDIN_FILENO) < 0) {
    err_sys("tty_reset() error");
  }
  if (i <= 0) {
    err_sys("read() error");
  }

  exit(0);
}
