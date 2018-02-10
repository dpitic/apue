/*
 * This function is used by a server to wait for a client's connected request
 * to arrive.  When one arrives, the system automatically creates a new UNIX
 * domain socket, connects it to the client's socket, and returns the new
 * socket to the server.  Additionally, the effective user ID of the client is
 * stored in the memory to which uidptr points.
 */
#include "apue.h"
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#define STALE 30 /* client's name can't be older than this (sec.) */

/**
 * Wait for a client connection to arrive, and accept it.
 * @param listenfd UNIX socket domain file descriptor the server listens on.
 * @param uidptr client's user ID.
 * @return new file descriptor on success, < 0 on error.
 */
int serv_accept(int listenfd, uid_t *uidptr) {
  int clifd, err, rval;
  socklen_t len;
  time_t staletime;
  struct sockaddr_un un;
  struct stat statbuf;
  char *name;

  /* Allocate enough space for longest name plus terminating null */
  if ((name = malloc(sizeof(un.sun_path) + 1)) == NULL) {
    return (-1);
  }
  len = sizeof(un);

  /*
   * Server blocks in accept() waiting for the client to call cli_conn().
   * accept() returns a new descriptor that is connected to the client.  Also,
   * the pathname that the client assigned to its socket (the name that
   * contained the client's process ID) is returned by accept() through its
   * second argument (pointer to struct sockaddr).
   */
  if ((clifd = accept(listenfd, (struct sockaddr *)&un, &len)) < 0) {
    free(name);
    return (-2); /* often errno=EINTR, if signal caught */
  }

  /* Obtain the client's uid from its calling address */
  len -= offsetof(struct sockaddr_un, sun_path); /* len of pathname */
  memcpy(name, un.sun_path, len);
  name[len] = 0; /* null terminate */
  /* Verify the pathname is a socket and that permissions allow only u+rwx */
  if (stat(name, &statbuf) < 0) {
    rval = -3;
    goto errout;
  }

#ifdef S_ISSOCK /* not defined for SVR4 */
  if (S_ISSOCK(statbuf.st_mode) == 0) {
    rval = -4; /* not a socket */
    goto errout;
  }
#endif

  if ((statbuf.st_mode & (S_IRWXG | S_IRWXO)) ||
      (statbuf.st_mode & S_IRWXU) != S_IRWXU) {
    rval = -5; /* is not rwx------ */
    goto errout;
  }

  /* Ensure the socket is not stale */
  staletime = time(NULL) - STALE;
  if (statbuf.st_atime < staletime || statbuf.st_ctime < staletime ||
      statbuf.st_mtime < staletime) {
    rval = -6; /* i-node is too old */
    goto errout;
  }

  /* Assume the client (effective UID) is the owner of the socket */
  if (uidptr != NULL) {
    *uidptr = statbuf.st_uid; /* return uid of caller */
  }
  unlink(name); /* pathname no longer needed */
  free(name);
  return (clifd);

errout:
  err = errno;
  close(clifd);
  free(name);
  errno = err;
  return (rval);
}
