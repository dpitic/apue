#include "apue.h"
#include <termios.h>

#ifdef LINUX
#define OPTSTR "+d:einv"
#else
#define OPTSTR "d:einv"
#endif

#define USAGE "Usage: %s [ -d driver -einv ] program [ arg ... ]"

static void set_noecho(int);    /* at the end of this file */
void do_driver(char *);         /* in the file driver.c */
void loop(int, int);            /* in the file loop.c */

/*
 * The pty program is used to enable running a program as follows:
 * 
 *    $ pty prog arg1 arg2
 * 
 * instead of simply:
 * 
 *    $ prog arg1 arg2
 * 
 * The pty program executes a program in a session of its own, connected to a
 * pseudo terminal.
 */
int main(int argc, char *argv[]) {
  int fdm, c, ignoreeof, interactive, noecho, verbose;
  pid_t pid;
  char *driver;
  char slave_name[20];
  struct termios orig_termios;
  struct winsize size;

  interactive = isatty(STDIN_FILENO);
  ignoreeof = 0;
  noecho = 0;
  verbose = 0;
  driver = NULL;

  opterr = 0;   /* don't want getopt() writing to stderr */
  while ((c = getopt(argc, argv, OPTSTR)) != EOF) {
    switch (c) {
    case 'd':   /* driver for stdin/stdout */
      driver = optarg;
      break;
    case 'e':   /* noecho for slave pty's line discipline */
      noecho = 1;
      break;
    case 'i':   /* ignore EOF on standard input */
      ignoreeof = 1;
      break;
    case 'n':   /* not interactive */
      interactive = 0;
      break;
    case 'v':   /* verbose */
      verbose = 1;
      break;
    case '?':
      err_quit(USAGE, argv[0]);
    }
  }

  if (optind >= argc) {
    err_quit(USAGE, argv[0]);
  }

  if (interactive) {    /* get current termios and window size */
    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) {
      err_sys("tcgetattr() error on stdin");
    }
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) &size) < 0) {
      err_sys("TIOCGWINSZ error");
    }
    pid = pty_fork(&fdm, slave_name, sizeof(slave_name), &orig_termios, &size);
  } else {
    pid = pty_fork(&fdm, slave_name, sizeof(slave_name), NULL, NULL);
  }

  if (pid < 0) {
    err_sys("fork() error");
  } else if (pid == 0) {    /* child */
    if (noecho) {
      set_noecho(STDIN_FILENO);   /* stdin is slave pty */
    }
    /*
     * Execute the program specified in the command line, with all remaining
     * command-line arguments passed as arguments to this program.
     */
    if (execvp(argv[optind], &argv[optind]) < 0) {
      err_sys("Can't execute: %s", argv[optind]);
    }
  }

  /* Parent */
  if (verbose) {
    fprintf(stderr, "Slave name = %s\n", slave_name);
    if (driver != NULL) {
      fprintf(stderr, "Driver = %s\n", driver);
    }
  }

  if (interactive && driver == NULL) {
    if (tty_raw(STDIN_FILENO) < 0) {    /* user's tty to raw mode */
      err_sys("tty_raw() error");
    }
    if (atexit(tty_atexit) < 0) {       /* reset user's tty on exit */
      err_sys("atexit() error");
    }
  }

  if (driver) {
    do_driver(driver);    /* changes our stdin/stdout */
  }

  loop(fdm, ignoreeof);   /* copies stdin -> ptym, ptym -> stdout */

  exit(0);
}

/**
 * Turn off echo (for slave pty).
 * @param fd open file descriptor of terminal.
 */
static void set_noecho(int fd) {
  struct termios stermios;

  if (tcgetattr(fd, &stermios) < 0) {
    err_sys("tcgetattr() error");
  }

  stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);

  /*
   * Also turn off NL to CR/NL mapping on output.
   */
  stermios.c_oflag &= ~(ONLCR);

  if (tcsetattr(fd, TCSANOW, &stermios) < 0) {
    err_sys("tcsetattr() error");
  }
}

