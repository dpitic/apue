/*
 * This program implements the ruptimed server using file descriptors, as an
 * alternative to the original implementation that read the output of the uptime
 * command, which it then sent to the client.  Instead, the server arranges to
 * have the stdout and stderr of the uptime command be the socket endpoint
 * connected to the client.  The advantage of using file descriptors to access
 * sockets is that it allows program that know nothing about networking to be
 * used in a networked environment.
 */
#include "apue.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <syslog.h>

#define QLEN 10

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

extern int initserver(int, const struct sockaddr *, socklen_t, int);

void serve(int sockfd) {
  int clfd, status;
  pid_t pid;

  set_cloexec(sockfd);
  for (;;) {
    if ((clfd = accept(sockfd, NULL, NULL)) < 0) {
      syslog(LOG_ERR, "ruptimed: accept() error: %s", strerror(errno));
      exit(1);
    }
    if ((pid = fork()) < 0) {
      syslog(LOG_ERR, "ruptimed: fork() error: %s", strerror(errno));
      exit(1);
    } else if (pid == 0) { /* child */
      /*
       * The parent called daemonize(), so STDIN_FILENO, STDOUT_FILENO, and
       * STDERR_FILENO are already open to /dev/null.  Thus, the call to close()
       * doesn't need to be protected by checks that clfd isn't already equal
       * to one of these values.
       */
      if (dup2(clfd, STDOUT_FILENO) != STDOUT_FILENO ||
          dup2(clfd, STDERR_FILENO) != STDERR_FILENO) {
        syslog(LOG_ERR, "ruptimed: unexpected error");
        exit(1);
      }
      close(clfd);
      execl("/usr/bin/uptime", "uptime", (char *)0);
      syslog(LOG_ERR, "ruptimed: unexpected return from exec(): %s",
             strerror(errno));
    } else {
      close(clfd);
      waitpid(pid, &status, 0);
    }
  }
}

int main(int argc, char *argv[]) {
  struct addrinfo *ailist, *aip;
  struct addrinfo hint;
  int sockfd, err, n;
  char *host;

  if (argc != 1) {
    err_quit("Usage: %s", argv[0]);
  }
  if ((n = sysconf(_SC_HOST_NAME_MAX)) < 0) {
    n = HOST_NAME_MAX; /* best guess */
  }
  if ((host = malloc(n)) == NULL) {
    err_sys("malloc() error");
  }
  if (gethostname(host, n) < 0) {
    err_sys("gethostname() error");
  }

  daemonize("ruptimed");
  memset(&hint, 0, sizeof(hint));
  hint.ai_flags = AI_CANONNAME;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_canonname = NULL;
  hint.ai_addr = NULL;
  hint.ai_next = NULL;

  if ((err = getaddrinfo(host, "ruptime", &hint, &ailist)) != 0) {
    syslog(LOG_ERR, "ruptimed: getaddrinfo() error: %s", gai_strerror(err));
    exit(1);
  }

  for (aip = ailist; aip != NULL; aip->ai_next) {
    if ((sockfd = initserver(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen,
                             QLEN)) >= 0) {
      serve(sockfd);
      exit(0);
    }
  }
  exit(1);
}
