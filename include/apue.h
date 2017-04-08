/*
 * Header file for code in the book. This header is to be included before all
 * standard system headers.
 */
 #ifndef _APUE_H
 #define _APUE_H

#include <sys/types.h>      /* some systems still require this */
#include <sys/stat.h>
#include <sys/termios.h>    /* for winsize */
#if defined(MACOS) || !defined(TIOCGWINSZ)
#include <sys/ioctl.h>
#endif

#include <stdio.h>          /* for convenience */
#include <stdlib.h>         /* for convenience */
#include <stddef.h>         /* for offsetof */
#include <string.h>         /* for convenience */
#include <unistd.h>         /* for convenience */
#include <signal.h>         /* for SIG_ERR */

#define MAXLINE 4096        /* maximum line length */

/*
 * Prototypes for custom functions used in the book.
 */
 void err_quit(const char *, ...) __attribute__((noreturn));
 void err_sys(const char *, ...) __attribute__((noreturn));
 void err_ret(const char *, ...);

 #endif /* _APUE_H */