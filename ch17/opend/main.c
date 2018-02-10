/*
 * Open Server main function, which defines the global variables, processes the
 * command-line options, and calls the function loop().  If the server is
 * invoked with the -d option, the server runs interactively (debug mode)
 * instead of as a daemon.
 */
#include "opend.h"
#include <syslog.h>

int debug, oflag, client_size, log_to_stderr;
char errmsg[MAXLINE];
char *pathname;
Client *client = NULL;

int main(int argc, char *argv[]) {
  int c;

  log_open("open.serv", LOG_PID, LOG_USER);

  opterr = 0; /* don't want getopt() writing to stderr */
  while ((c = getopt(argc, argv, "d")) != EOF) {
    switch (c) {
    case 'd':
      debug = log_to_stderr = 1;
      break;
    case '?':
      err_quit("Unrecognised option: -%c", optopt);
    }
  }

  if (debug) {
    daemonize("opend");
  }

  loop(); /* never returns */
}
