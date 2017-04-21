/*
 * Copy from stdin to stdout. The implementation uses the Standard C Library
 * I/O functions getc() and putc(), which provide a buffered interface to the
 * unbuffered I/O system call functions read() and write().
 * Using standard library I/O functions relieves us from having to choose
 * optimal buffer sizes.
 */
#include "apue.h"

int main(void)
{
    int        c;      /* character read and to be written */

    while ((c = getc(stdin)) != EOF) {
        if (putc(c, stdout) == EOF) {
            err_sys("output error");
        }
    }

    if (ferror(stdin)) {
        err_sys("input error");
    }

    exit(0);
}
