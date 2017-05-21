/*
 * This program demonstrates the use of several of the time functions. It shows
 * how strftime() can be used to print strings containing the current date and
 * time.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
  time_t t;
  struct tm *tmp;   /* broken-down time structure */
  char buf1[16];
  char buf2[64];

  time(&t);             /* get current calendar time (date and time) */
  tmp = localtime(&t);  /* convert to broken-down time structure */
  
  /* Format time string representation from the broken-down time structure */
  if (strftime(buf1, 16, "Time and date: %r, %a, %b, %d, %Y", tmp) == 0) {
    printf("Buffer length 16 is too small\n");
  } else {
    printf("%s\n", buf1);
  }
  if (strftime(buf2, 64, "Time and date: %r, %a, %b, %d, %Y", tmp) == 0) {
    printf("Buffer length 64 is too small\n");
  } else {
    printf("%s\n", buf2);
  }
  exit(0);
}