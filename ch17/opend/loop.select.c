#include "opend.h"
#include <sys/select.h>

/**
 * Open Server infinite loop, implemented using select(), which is a system call
 * for synchronous I/O multiplexing that examines I/O descriptor sets to see if
 * some of their descriptors are ready for reading, writing, or have exceptional
 * conditions pending.  On return, it replaces the given descriptor sets with
 * the subsets consisting of those descriptors that are ready for the requested
 * operation, and returns the total number of ready descriptors in all of the
 * sets.  Various FD_xxx() macros are provided for manipulating descriptor sets.
 */
void loop(void) {
  int i, n, maxfd, maxi, listenfd, clifd, nread;
  char buf[MAXLINE];
  uid_t uid;
  fd_set rset, allset; /* read and in use file descriptor sets */

  FD_ZERO(&allset); /* initialise in use descriptor set to null set */

  /* Create server's endpoint (fd) to listen for client requests on */
  if ((listenfd = serv_listen(CS_OPEN)) < 0) {
    log_sys("serv_listen() error");
  }
  FD_SET(listenfd, &allset); /* add client listen fd to in use set */
  maxfd = listenfd;          /* number of fd for select() */
  maxi = -1;                 /* maximum client index in client[] array */

  /* Infinite server (daemon) loop listening for client connections */
  for (;;) {
    rset = allset; /* rset gets modified each time around */
    /* Get descriptors that are ready to be read */
    if ((n = select(maxfd + 1, &rset, NULL, NULL, NULL)) < 0) {
      log_sys("select() error");
    }

    /* Check whether client listen fd is part of read set */
    if (FD_ISSET(listenfd, &rset)) {
      /* Accept new client request (client has called cli_conn()) */
      if ((clifd = serv_accept(listenfd, &uid)) < 0) {
        log_sys("serv_accept() error: %d", clifd);
      }
      i = client_add(clifd, uid); /* add client to client[] array */
      FD_SET(clifd, &allset);     /* add client fd to in use fd set */
      if (clifd > maxfd) {
        maxfd = clifd; /* update number of fd for select() */
      }
      if (i > maxi) {
        maxi = i; /* max index in client[] array */
      }
      log_msg("New connection: uid %d, fd %d", uid, clifd);
      continue;
    }

    /* Traverse the client[] array */
    for (i = 0; i <= maxi; i++) {
      if ((clifd = client[i].fd) < 0) {
        continue; /* no valid fd at that array index */
      }
      if (FD_ISSET(clifd, &rset)) {
        /* Read argument buffer from client */
        if ((nread = read(clifd, buf, MAXLINE)) < 0) {
          log_sys("read() error on fd %d", clifd);
        } else if (nread == 0) {
          log_msg("Closed: uid %d, fd %d", client[i].uid, clifd);
          client_del(clifd);      /* client has closed connection */
          FD_CLR(clifd, &allset); /* remove client from all fd set */
          close(clifd);
        } else { /* process client's request */
          handle_request(buf, nread, clifd, client[i].uid);
        }
      }
    }
  }
}
