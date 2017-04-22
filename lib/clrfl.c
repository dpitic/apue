/*
 * This function clears one or more of the file status flags for a descriptor.
 *
 * When modifying either the file descriptor flags or the file status flags,
 * the existing flag value must be obtained, modified as desired, and then set
 * to the new flag value. It is not possible to simply issue an F_SETFD or
 * F_SETFL command, as this could turn off bits that were previously set.
 */
#include "apue.h"
#include <fcntl.h>

void clr_fl(int fd, int flags) /* flags are file status flags to turn off */
{
  int val;

  if ((val = fcntl(fd, F_GETFL, 0)) < 0) {
    err_sys("fcntl F_GETFL error");
  }

  val &= ~flags; /* turn off flags */

  if (fcntl(fd, F_SETFL, val) < 0) {
    err_sys("fcntl F_SETFL error");
  }
}