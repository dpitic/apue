/*
 * Copy from stdin to stdout. The implementation uses unbuffered I/O functions
 * such as read() and write(). These functions are system calls.
 */
 #include "apue.h"

 #define BUFFSIZE   4096    /* read buffer size in bytes */

 int main(void)
 {
     int    n;              /* number of bytes read and to be written */
     char   buf[BUFFSIZE];

     while ((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0) {
         if (write(STDOUT_FILENO, buf, n) != n) {
             err_sys("write error");
         }
     }

     if (n < 0) {
         err_sys("read error");
     }

     exit(0);
 }