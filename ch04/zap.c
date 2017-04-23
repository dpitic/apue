/*
 * This program truncates files to zero length using the O_TRUNC option of the
 * open() function, but does not change their access time or modification time.
 * To do this, the program first obtains the times with the stat() function,
 * truncates the files, and then resets the times with the futimens() function.
 */
#include "apue.h"
#include <fcntl.h>

int main(int argc, char const *argv[]) {
  int i, fd;
  struct stat statbuf;
  struct timespec times[2];

  for (i = 1; i < argc; i++) {
    if (stat(argv[i], &statbuf) < 0) {  /* fetch current times */
      err_ret("%s: stat error", argv[i]);
      continue;
    }
    if ((fd = open(argv[i], O_RDWR | O_TRUNC)) < 0) {
      err_ret("%s: open error", argv[i]);
      continue;
    }
    times[0] = statbuf.st_atim;
    times[1] = statbuf.st_mtim;
    if (futimens(fd, times) < 0) {  /* reset times */
      err_ret("%s: futimens error", argv[i]);
    }
    close(fd);
  }
  exit(0);
}
