/*
 * This program copies a file using only the read() and write() functions. It
 * reads from standard input and writes to standard output, assuming that these
 * have been set up by the shell before the program is executed.
 *
 * This program works for both text files and binary files, since there is no
 * difference between the two to the UNIX kernel.
 */
#include "apue.h"

#define BUFFSIZE 4096

int main(void)
{
	int n;					/* number of bytes read and written */
	char buf[BUFFSIZE];		/* read and write buffer */

	while ((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0) {
		if (write(STDOUT_FILENO, buf, n) != n)
		{
			err_sys("write error");
		}
	}

	if (n < 0)
	{
		err_sys("read error");
	}

	exit(0);
}