/*
 * This is a sample implementation of the ttyname() function.  It has to search
 * all the device entries, looking for a match.
 */
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

struct devdir {
  struct devdir *d_next;
  char *d_name;
};

static struct devdir *head;
static struct devdir *tail;
static char pathname[_POSIX_PATH_MAX + 1];

static void add(char *dirname) {
  struct devdir *ddp;
  int len;

  len = strlen(dirname);

  /*
   * Skip ., .., and /dev/fd.
   */
  if ((dirname[len - 1] == '.') &&
      (dirname[len - 2] == '/' ||
       (dirname[len - 2] == '.' && dirname[len - 3] == '/'))) {
    return;
  }
  if (strcmp(dirname, "/dev/fd") == 0) {
    return;
  }
  if ((ddp = malloc(sizeof(struct devdir))) == NULL) {
    return;
  }
  if ((ddp->d_name = strdup(dirname)) == NULL) {
    free(ddp);
    return;
  }

  ddp->d_next = NULL;
  if (tail == NULL) {
    head = ddp;
    tail = ddp;
  } else {
    tail->d_next = ddp;
    tail = ddp;
  }
}
