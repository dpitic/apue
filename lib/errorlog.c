/*
 * Error routines for programs that can run as a daemon.
 */

#include <errno.h>  /* for definition of errno */
#include <stdarg.h> /* ISO C variable arguments */
#include <syslog.h>
#include "apue.h"

static void log_doit(int, int, int, const char *, va_list ap);

/**
 * Caller must define and set this: nonzero if interactive (i.e. not daemon), so
 * that error messages are sent to standard error. If set to 0, then the
 * syslog() facility will be used.
 */
extern int log_to_stderr;

/**
 * Initialise syslog(), if running as a daemon.
 *
 * @param[in]  ident     Pointer to null-terminated character string that is
 *                       prepended to every message.
 * @param[in]  option    Bit field specifying the logging option. This is the
 *                       logopt argument passed to openlog().
 * @param[in]  facility  Encodes a default facility to be assigned to all
 *                       messages that do no have an explicit facility encoded.
 *                       This is the facility parameter passed to openlog().
 */
void log_open(const char *ident, int option, int facility) {
  if (log_to_stderr == 0) {
    openlog(ident, option, facility);
  }
}

/**
 * Nonfatal error related to a system call. Print a message with the system's
 * errno value and return.
 * @param[in]  fmt        Variable length argument format specifier string.
 */
void log_ret(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  log_doit(1, errno, LOG_ERR, fmt, ap);
  va_end(ap);
}

/**
 * Fatal error related to a system call. Print a message and terminate.
 * @param[in]  fmt        Variable length argument format specifier string.
 */
void log_sys(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  log_doit(1, errno, LOG_ERR, fmt, ap);
  va_end(ap);
  exit(2);
}

/**
 * Nonfatal error unrelated to a system call. Print a message and return.
 * @param[in]  fmt        Variable length argument format specifier string.
 */
void log_msg(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  log_doit(0, 0, LOG_ERR, fmt, ap);
  va_end(ap);
}

/**
 * Fatal error unrelated to a system call. Print a message and terminate.
 * @param      fmt        Variable length argument format specifier string.
 */
void log_quit(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  log_doit(0, 0, LOG_ERR, fmt, ap);
  va_end(ap);
  exit(2);
}

/**
 * Fatal error related to a system call. Error number passed as an explicit
 * parameter.  Print a message and terminate.
 * @param[in]  error      Error number
 * @param[in]  fmt        Variable length argument format specifier string.
 */
void log_exit(int error, const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  log_doit(1, error, LOG_ERR, fmt, ap);
  va_end(ap);
  exit(2);
}

/**
 * Print a message and return to caller. Caller specifies errnoflag and
 * priority.
 * @param[in]  errnoflag  Flag used to specify if errno is set. 0 = errno not
 *                        set; otherwise errno set.
 * @param[in]  error      Error number integer passed to strerror()
 * @param[in]  priority   Message tagged with priority.
 * @param[in]  fmt        Variable length argument format specifier string.
 */
static void log_doit(int errnoflag, int error, int priority, const char *fmt,
                     va_list ap) {
  char buf[MAXLINE];

  vsnprintf(buf, MAXLINE - 1, fmt, ap);
  if (errnoflag) {
    snprintf(buf + strlen(buf), MAXLINE + strlen(buf) - 1, ": %s",
             strerror(error));
  }
  strcat(buf, "\n");
  if (log_to_stderr) {
    fflush(stdout);
    fputs(buf, stderr);
    fflush(stderr);
  } else {
    syslog(priority, "%s", buf);
  }
}
