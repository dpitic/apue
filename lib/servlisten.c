/*
 * This function can be used by a server to announce its willingness to listen
 * for client connect requests on a well-known name (some pathname in the file
 * system).  Clients will use this name when they want to connect to the server.
 * The return value is the server's UNIX domain socket used to receive client
 * connection requests.
 */
#include "apue.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define QLEN 10

/**
 * Create a server endpoint of a connection with a well-known pathname, name.
 * @param name well-known pathname in the file system that the server listens
 * to.
 * @return fd if all OK, < 0 on error.
 */
int serv_listen(const char *name) {
  int fd, len, err, rval;
  struct sockaddr_un un;

  if (strlen(name) >= sizeof(un.sun_path)) {
    errno = ENAMETOOLONG;
    return(-1);
  }

  /* Create a UNIX domain stream socket */
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    return(-2);
  }

  unlink(name);       /* in case it already exists */

  /* Fill in socket address structure */
  memset(&un, 0, sizeof(un));
  un.sun_family = AF_UNIX;
  strcpy(un.sun_path, name);
  len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

  /* Bind the name to the descriptor */
  if (bind(fd, (struct sockaddr *)&un, len) < 0) {
    rval = -3;
    goto errout;
  }

  /*
   * When a connection request from a client arrives, the server calls the
   * serv_accept() function.
   */
  if (listen(fd, QLEN) < 0) {  /* tell kernel we're a server */
    rval = -4;
    goto errout;
  }
  return(fd);

 errout:
  err = errno;
  close(fd);
  errno = err;
  return(rval);
}
