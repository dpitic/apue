/*
 * This program opens a file and then unlinks it. The program then goes to
 * sleep for 15 seconds before terminating.
 *
 * The behavour of this program is different to the example behaviour shown in
 * the book. The book shows that the tempfile is created and shows in the
 * directory listing. However, by immediately unlinking the file, the temporary
 * file does not show as a file listing in the directory.
 */
#include "apue.h"
#include <fcntl.h>

int main(void) {
  /* 
   * Extra flags (and mode) are required to create the new tempfile, otherwise
   * the file is not created. The implementation in the book lacks these flags
   * and consequently, the tempfile is not created. 
   */
  if (open("tempfile", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR) < 0) {
    err_sys("open error");
  }
  /*
   * Immediately unlinking the file prevents the file from showing in the
   * directory listing. However, the file is available for reading and writing
   * until the program terminates.
   */
  if (unlink("tempfile") < 0) {
    err_sys("unlink error");
  }
  printf("file unlinked\n");
  sleep(15);
  printf("done\n");
  exit(0);
}