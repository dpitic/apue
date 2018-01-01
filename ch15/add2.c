/*
 * Simple filter that reads two numbers from its standard input, computes their
 * sum, and writes the sum to its standard output.  This filter is used as an
 * example coprocess, where another program writes and reads from this filter.
 */
#include "apue.h"

int main(void)
{
  int n, int1, int2;
  char line[MAXLINE];

  while ((n = read(STDIN_FILENO, line, MAXLINE)) > 0) {
    line[n] = 0; /* null terminate string */
    if (sscanf(line, "%d%d", &int1, &int2) == 2) {
      sprintf(line, "%d\n", int1 + int2);
      n = strlen(line);
      if (write(STDOUT_FILENO, line, n) != n) {
        err_sys("write() error");
      }
    } else {
      if (write(STDOUT_FILENO, "Invalid args\n", 13) != 13) {
        err_sys("write() error");
      }
    }
  }
  exit(0);
}
