/*
 * This program shows how to use (and how not to use) the mkstemp() function.
 */
#include "apue.h"
#include <errno.h>

void make_temp(char *template);

int main() {
  char good_template[] = "/tmp/dirXXXXXX"; /* right way */
  char *bad_template = "/tmp/dirXXXXXX";   /* wrong way */

  printf("trying to crete first temp file ...\n");
  /*
   * good_template is allocated on the stack and mkstemp() can modify the
   * string.
   */
  make_temp(good_template);
  printf("trying to create second temp file ...\n");
  /*
   * Only the pointer for bad_template is allocated on the stack; the compiler
   * allocates storage for the string in the read-only segment of the
   * executable. When mkstemp() tries to modify the string, a segmentation fault
   * occurs.
   */
  make_temp(bad_template);
  exit(0);
}

void make_temp(char *template) {
  int fd;
  struct stat sbuf;

  if ((fd = mkstemp(template)) < 0) {
    err_sys("can't create temp file");
  }
  printf("temp name = %s\n", template);
  close(fd);
  if (stat(template, &sbuf) < 0) {
    if (errno == ENOENT) {
      printf("file doesn't exite\n");
    } else {
      printf("file exists\n");
      unlink(template);
    }
  }
}
