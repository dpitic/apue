/*
 * This program implements the server that provides the uptime command's output
 * to the client program (ruptime).
 */
#include "apue.h"
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <syslog.h>

#define BUFLEN 128
#define QLEN 10

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

extern int initserver(int, const struct sockaddr *, socklen_t, int);

void serve(int sockfd) {
  int clfd;
  FILE *fp;
  char buf[BUFLEN];

  set_cloexec(sockfd);
  for (;;) {
    if ((clfd = accept(sockfd, NULL, NULL)) < 0) {
      syslog(LOG_ERR, "ruptimed: accept() error: %s", strerror(errno));
      exit(1);
    }
    set_cloexec(clfd);

    /* Read the output of uptime and send it to the client */
    if ((fp = popen("/usr/bin/uptime", "r")) == NULL) {
      sprintf(buf, "Error: %s\n", strerror(errno));
      send(clfd, buf, strlen(buf), 0);
    } else {
      while (fgets(buf, BUFLEN, fp) != NULL) {
        send(clfd, buf, strlen(buf), 0);
      }
      pclose(fp);
    }
    close(clfd);
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
    syslog(LOG_ERR, "ruptimed: getaddrinfo() error %s", gai_strerror(err));
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
