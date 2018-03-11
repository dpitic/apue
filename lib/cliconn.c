/*
 * This function is used by a client to connect to a server.  The name argument
 * specified by the client must be the same name that was advertised by the
 * server's call to serv_listen().  On return, the client gets a file descriptor
 * connected to the server.
 */
#include "apue.h"
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define CLI_PATH "/var/tmp/"
#define CLI_PERM S_IRWXU /* rwx for user only */

/**
 * Create a client endpoint and connect to a server.
 * @param name the same name advertised by the server.
 * @return file descriptor connected to the server.
 */
int cli_conn(const char *name) {
  int fd, len, err, rval;
  struct sockaddr_un un, sun;
  int do_unlink = 0;

  if (strlen(name) >= sizeof(un.sun_path)) {
    errno = ENAMETOOLONG;
    return (-1);
  }

  /* Create client's end UNIX domain stream socket */
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    return (-1);
  }

  /* Fill socket address structure with client's address */
  memset(&un, 0, sizeof(un));
  un.sun_family = AF_UNIX;
  sprintf(un.sun_path, "%s%05ld", CLI_PATH, (long)getpid());
  len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

  unlink(un.sun_path); /* in case it already exists */
  /*
   * Don't let the system choose a default address for the client, because the
   * server wouldn't be able to distinguish between clients (if we don't
   * explicitly bind a name to a UNIX domain socket, the kernel implicitly binds
   * an address to it on our behalf and no file is created in the file system to
   * represent the socket).  Instead, bind the clients address; this step is
   * usually not taken when developing a client program that uses sockets.
   */
  if (bind(fd, (struct sockaddr *)&un, len) < 0) {
    rval = -2;
    goto errout;
  }
  if (chmod(un.sun_path, CLI_PERM) < 0) {
    rval = -3;
    do_unlink = 1;
    goto errout;
  }

  /* Fill socket address structure with server's address */
  memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;
  strcpy(sun.sun_path, name);
  len = offsetof(struct sockaddr_un, sun_path) + strlen(name);
  /* Initiate connection to server */
  if (connect(fd, (struct sockaddr *)&sun, len) < 0) {
    rval = -4;
    do_unlink = 1;
    goto errout;
  }
  return (fd);

errout:
  err = errno;
  close(fd);
  if (do_unlink) {
    unlink(un.sun_path);
  }
  errno = err;
  return (rval);
}
