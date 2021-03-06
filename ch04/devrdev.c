/*
 * This program prints the device number for each command-line argument. If the
 * argument refers to a character special file or a block special file, the
 * st_rdev value for the special file is printed.
 */
#include "apue.h"
#ifdef SOLARIS
#include <sys/mkdev.h>
#endif
#ifdef _GNU_SOURCE
#include <sys/sysmacros.h>
#endif

int main(int argc, char const *argv[]) {
  int i;
  struct stat buf;

  for (i = 1; i < argc; i++) {
    printf("%s: ", argv[i]);
    if (stat(argv[i], &buf) < 0) {
      err_ret("stat error");
      continue;
    }

    printf("dev = %d/%d", major(buf.st_dev), minor(buf.st_dev));

    if (S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode)) {
      printf(" (%s) rdev = %d/%d",
             (S_ISCHR(buf.st_mode)) ? "character" : "block", major(buf.st_rdev),
             minor(buf.st_rdev));
    }
    printf("\n");
  }

  exit(0);
}
