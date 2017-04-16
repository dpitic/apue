/*
 * This program creates a file with a hole in it. This happens when the file's
 * offset is greater than the file's current size, and the next write to the
 * file will extend the file. Any bytes that have not been written are read
 * back as 0.
 *
 * A hole isn't required to have storage backing it on disk. Depending on the
 * file system implementation, when you write after seeking past the end of a
 * file, new disk blocks might be allocated to store the data, but there is no
 * need to allocate blocks for the data between the old end of file and the
 * locatino where you start writing.
 */
 #include "apue.h"
 #include <fcntl.h>

 char buf1[] = "abcdefghij";	/* 10 bytes */
 char buf2[] = "ABCDEFGHIJ";	/* 10 bytes */

 int main(void)
 {
     int fd;

     if ((fd = creat("file.hole", FILE_MODE)) < 0) {
         err_sys("creat error");
     }

     if (write(fd, buf1, 10) != 10) {
         err_sys("buf1 write error");
     }
     /* offset now = 10 */

     if (lseek(fd, 16384, SEEK_SET) == -1) {
         err_sys("lseek error");
     }
     /* offset now = 16384 */

     if (write(fd, buf2, 10) != 10) {
     	err_sys("buf2 write error");
     }
     /* offset now = 16394 */

     exit(0);
 }
