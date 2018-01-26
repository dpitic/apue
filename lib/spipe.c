/*
 * This function uses the socketpair() function to create a pair of connected
 * UNIX domain stream sockets.  These act like a full-duplex pipe: both ends
 * are open for reading and writing; these will be referred to as "fd-pipes",
 * to distinguish them from normal, half-duplex pipes.
 */
#include "apue.h"
#include <sys/socket.h>

/*
 * Return a full-duplex pipe (a UNIX domain socket) with the two file
 * descriptors returned in fd[0] and fd[1];
 */

int fd_pipe(int fd[2]) { return (socketpair(AF_UNIX, SOCK_STREAM, 0, fd)); }
