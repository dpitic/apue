/*
 * Demonstration of the error functions. This program prints the error message
 * for permission problem.
 */
 #include "apue.h"
 #include <errno.h>

 int main(int argc, char *argv[])
 {
     fprintf(stderr, "EACCES: %s\n", strerror(EACCES));
     errno = ENOENT;
     perror(argv[0]);
     exit(0);
 }