/*
 * This program demonstrates how a signle threaded daemon can catch SIGHUP and
 * reread its configuration file.
 */
#include "apue.h"
#include <errno.h>
#include <syslog.h>

extern int lockfile(int);
extern int already_running(void);

void reread(void) { /* Re-read configuration file */ }

void sigterm(int signo) {
  syslog(LOG_INFO, "Got SIGTERM; exiting");
  exit(0);
}

void sighup(int signo) {
  syslog(LOG_INFO, "Re-reading configuration file");
  reread();
}

int main(int argc, char *argv[argc]) {
  char *cmd;
  struct sigaction sa;

  if ((cmd = strrchr(argv[0], '/')) == NULL) {
    cmd = argv[0];
  } else {
    cmd++;
  }

  /*
   * Become a daemon.
   */
  daemonize(cmd);

  /*
   * Make sure only one copy of the daemon is running.
   */
  if (already_running()) {
    syslog(LOG_ERR, "Daemon already running");
    exit(1);
  }

  /*
   * Handle signals of interest.
   */
  sa.sa_handler = sigterm;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGHUP);
  sa.sa_flags = 0;
  if (sigaction(SIGTERM, &sa, NULL) < 0) {
    syslog(LOG_ERR, "Can't catch SIGTERM: %s", strerror(errno));
    exit(1);
  }
  sa.sa_handler = sighup;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGTERM);
  sa.sa_flags = 0;
  if (sigaction(SIGHUP, &sa, NULL) < 0) {
    syslog(LOG_ERR, "Can't catch SIGHUP: %s", strerror(errno));
    exit(1);
  }

  /*
   * Proceed with the rest of the daemon.
   */
  /* ... */
  exit(0);
}
