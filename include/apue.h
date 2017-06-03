/*
 * Header file for code in the book. This header is to be included before all
 * standard system headers.
 */
#ifndef _APUE_H
#define _APUE_H

#define _POSIX_C_SOURCE 200809L

#if defined(SOLARIS) /* Solaris 10 */
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 700
#endif

#include <sys/stat.h>
#include <sys/termios.h> /* for winsize */
#include <sys/types.h>   /* some systems still require this */
#if defined(MACOS) || !defined(TIOCGWINSZ)
#include <sys/ioctl.h>
#endif

#include <signal.h> /* for SIG_ERR */
#include <stddef.h> /* for offsetof */
#include <stdio.h>  /* for convenience */
#include <stdlib.h> /* for convenience */
#include <string.h> /* for convenience */
#include <unistd.h> /* for convenience */

#define MAXLINE 4096 /* maximum line length */

/*
 * Default file access permissions for new file.
 */
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/*
 * Prototypes for custom functions used in the book.
 */
char *path_alloc(size_t *); /* pathalloc.c */
long open_max(void);        /* openmax.c */

void set_fl(int, int); /* setfl.c */
void clr_fl(int, int);

void pr_exit(int);      /* prexit.c */

void err_quit(const char *, ...) __attribute__((noreturn));
void err_sys(const char *, ...) __attribute__((noreturn));
void err_ret(const char *, ...);
void err_dump(const char *, ...) __attribute__((noreturn));

#endif /* _APUE_H */
