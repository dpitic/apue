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

#if defined(BSD)     /* FreeBSD */
#include <termios.h> /* for winsize */
#else
#include <sys/termios.h> /* for winsize */
#endif

#include <sys/types.h> /* some systems still require this */
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

/* Simplified signal handler function prototype */
typedef void Sigfunc(int);

/*
 * Prototypes for custom functions used in the book.
 */
char *path_alloc(size_t *); /* pathalloc.c */
long open_max(void);        /* openmax.c */

void set_fl(int, int); /* setfl.c */
void clr_fl(int, int);

void pr_exit(int); /* prexit.c */

void pr_mask(const char *);           /* prmask.c */
Sigfunc *signal_intr(int, Sigfunc *); /* signalintr.c */

void daemonize(const char *); /* daemonize.c */

/* Simplified signal() prototype for cross-platform implementation */
/* Sigfunc *signal(int, Sigfunc *); */

int lock_reg(int, int, int, off_t, int, off_t); /* lockreg.c */

#define read_lock(fd, offset, whence, len)                                     \
  lock_reg((fd), F_SETLK, F_RDLCK, (offset), (whence), (len))
#define readw_lock(fd, offset, whence, len)                                    \
  lock_reg((fd), F_SETLKW, F_RDLCK, (offset), (whence), (len))
#define write_lock(fd, offset, whence, len)                                    \
  lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
#define writew_lock(fd, offset, whence, len)                                   \
  lock_reg((fd), F_SETLKW, F_WRLCK, (offset), (whence), (len))
#define un_lock(fd, offset, whence, len)                                       \
  lock_reg((fd), F_SETLK, F_UNLCK, (offset), (whence), (len))

/* Alternative definition of lockfile() */
/* #define lockfile(fd) write_lock((fd), 0, SEEK_SET, 0) */

pid_t lock_test(int, int, off_t, int, off_t); /* locktest.c */

#define is_read_lockable(fd, offset, whence, len)                              \
  (lock_test((fd), F_RDLCK, (offset), (whence), (len)) == 0)
#define is_write_lockable(fd, offset, whence, len)                             \
  (lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

void err_quit(const char *, ...) __attribute__((noreturn));
void err_cont(int, const char *, ...);
void err_sys(const char *, ...) __attribute__((noreturn));
void err_ret(const char *, ...);
void err_dump(const char *, ...) __attribute__((noreturn));
void err_exit(int, const char *, ...) __attribute__((noreturn));

void TELL_WAIT(void); /* parent/child from race conditions section */
void TELL_PARENT(pid_t);
void TELL_CHILD(pid_t);
void WAIT_PARENT(void);
void WAIT_CHILD(void);

#endif /* _APUE_H */
