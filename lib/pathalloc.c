/*
 * Portable dynamic storage allocation for a pathname.
 *
 * POSIX.1 provides the PATH_MAX limit, but this is inteterminate on some
 * implementations, and must be determined using pathconf(). The value returned
 * by pathconf() is the maximum size of a relative pathname, when the first
 * argument is the working directory.
 *
 * Versions of POSIX.1 prior to 2001 were unclear whether PATH_MAX included a
 * null byte at the end of the pathname. If the OS implementation conforms to
 * one of these prior version and doesn't conform to any version of the SUS
 * (which does require the terminating null byte to be included), we need to
 * add 1 to the amount of memory we allocate for a pathname.
 */
#include "apue.h"
#include <errno.h>
#include <limits.h>

#ifdef PATH_MAX
static long pathmax = PATH_MAX;
#else
static long pathmax = 0;
#endif

static long posix_version = 0;
static long xsi_version = 0;

/* If PATH_MAX is indeterminate, no guarantee this is adequate */
#define PATH_MAX_GUESS 1024

char *path_alloc(size_t *sizep) /* also return allocated size, if nonnull */
{
  char *ptr;
  size_t size;

  if (posix_version == 0) {
    posix_version = sysconf(_SC_VERSION);
  }
  if (xsi_version == 0) {
    xsi_version = sysconf(_SC_XOPEN_VERSION);
  }

  if (pathmax == 0) { /* first time through */
    errno = 0;
    if ((pathmax = pathconf("/", _PC_PATH_MAX)) < 0) {
      if (errno == 0) {
        pathmax = PATH_MAX_GUESS; /* it's indeterminate */
      } else {
        err_sys("pathconf error for _PC_PATH_MAX");
      }
    } else {
      pathmax++; /* add 1 since it's relative to root */
    }
  }

  /*
   * Before POSIX.1-2001, we aren't guaranteed that PATH_MAX includes the
   * terminating null byte. Same goes for XPG3.
   */
  if ((posix_version < 200112L) && (xsi_version < 4)) {
    size = pathmax + 1;
  } else {
    size = pathmax;
  }

  if ((ptr = malloc(size)) == NULL) {
    err_sys("malloc error for pathname");
  }

  if (sizep != NULL) {
    *sizep = size;
  }

  return (ptr);
}