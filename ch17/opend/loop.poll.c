#include "opend.h"
#include <poll.h>

#define NALLOC 10 /* # pollfd structs to alloc/realloc */

/**
 * Dynamically reallocate memory for the array of pollfd structures, which was
 * originally allocated in the main loop, in order to accommodate more elements.
 * The maximum number of clients is limited by the maximum number of open
 * descriptors possible.
 * @param pfd pointer to pollfd structure array to grow in size.
 * @param maxfd pointer to maximum size of pollfd structure array.
 * @return pointer to pollfd structure array that has been re-sized with an
 * additional number of elements equal to the original allocated size.
 */
static struct pollfd *grow_pollfd(struct pollfd *pfd, int *maxfd) {
  int i;
  int oldmax = *maxfd;
  int newmax = oldmax + NALLOC;

  if ((pfd = realloc(pfd, newmax * sizeof(struct pollfd))) == NULL) {
    err_sys("realloc() error");
  }
  /* Initialise new elements */
  for (i = oldmax; i < newmax; i++) {
    pfd[i].fd = -1; /* clear revents; also prevents this fd being checked */
    /* Initialise to look for new request from existing client (events) */
    pfd[i].events = POLLIN;
    pfd[i].revents = 0; /* events returned */
  }
  *maxfd = newmax;
  return (pfd);
}

/**
 * Open Server infinite loop, implemented using poll(), which is a system call
 * for synchronous I/O multiplexing that examines a set of file descriptors to
 * see if some of them are ready for I/O.
 */
void loop(void) {
  int i, listenfd, clifd, nread;
  char buf[MAXLINE];
  uid_t uid;
  struct pollfd *pollfd;
  int numfd = 1;
  int maxfd = NALLOC;

  if ((pollfd = malloc(NALLOC * sizeof(struct pollfd))) == NULL) {
    err_sys("malloc() error");
  }
  /* Initialise new elements */
  for (i = 0; i < NALLOC; i++) {
    pollfd[i].fd = -1; /* clear revents; also prevents this fd being checked */
    /* Initialise to look for new request from existing client (events) */
    pollfd[i].events = POLLIN;
    pollfd[i].revents = 0;
  }

  /* Obtain fd to listen for client request on */
  if ((listenfd = serv_listen(CS_OPEN)) < 0) {
    log_sys("serv_listen() error");
  }
  client_add(listenfd, 0); /* use [0] for listenfd */
  pollfd[0].fd = listenfd;

  for (;;) {
    /* Synchronous I/O multiplexing using poll() system call */
    if (poll(pollfd, numfd, -1) < 0) { /* block indefinitely */
      log_sys("poll() error");
    }

    /* Check arrival of new client connection on listenfd descriptor */
    if (pollfd[0].revents & POLLIN) {
      /* Accept new client request */
      if ((clifd = serv_accept(listenfd, &uid)) < 0) {
        log_sys("serv_accept() error: %d", clifd);
      }
      client_add(clifd, uid);

      /* Possibly increase the size of the pollfd array */
      if (numfd == maxfd) {
        pollfd = grow_pollfd(pollfd, &maxfd);
      }
      /* Add client to pollfd array */
      pollfd[numfd].fd = clifd;
      pollfd[numfd].events = POLLIN;
      pollfd[numfd].revents = 0;
      numfd++;
      log_msg("New connection: uid %d, fd %d", uid, clifd);
    }

    /*
     * Traverse pollfd array checking for events from existing clients
     *   1. New request from an existing client.
     *   2. Client connection termination.
     */
    for (i = 1; i < numfd; i++) {
      /* Check client connection termination event */
      if (pollfd[i].revents & POLLHUP) {
        goto hungup;
      } else if (pollfd[i].revents & POLLIN) { /* new request event */
        /* Read argument buffer from client */
        if ((nread = read(pollfd[i].fd, buf, MAXLINE)) < 0) {
          log_sys("read() error on fd %d", pollfd[i].fd);
        } else if (nread == 0) {
        hungup:
          /*
           * The client closed the connection while there is still data to be
           * read from the server's end of the connection.  Even though the
           * endpoint is marked as hung up, the server can still read all the
           * data queued on its end.  However, there is no point processing
           * the request because the server can't send any response back.  Just
           * close the connection to the client, throwing away any queued data.
           */
          log_msg("Closed: uid %d, fd %d", client[i].uid, pollfd[i].fd);
          client_del(pollfd[i].fd);
          close(pollfd[i].fd);
          if (i < (numfd - 1)) {
            /* Pack the array */
            pollfd[i].fd = pollfd[numfd - 1].fd;
            pollfd[i].events = pollfd[numfd - 1].events;
            pollfd[i].revents = pollfd[numfd - 1].revents;
            i--; /* recheck this entry */
          }
          numfd--;
        } else {
          handle_request(buf, nread, pollfd[i].fd, client[i].uid);
        }
      }
    }
  }
}
