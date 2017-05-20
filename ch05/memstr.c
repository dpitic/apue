/*
 * This program demonstrates how writes to a memory stream operate on a buffer
 * provided. It seeds the buffer with a known pattern to see how writes to the
 * stream behave.
 *
 * NOTE: At the time of writing, only Linux provides support for memory streams.
 * This is a case of the implementations not having caught up yet with the 
 * latest standards.
 */
#include "apue.h"

#define BSZ 48

int main(void) {
  FILE *fp;
  char buf[BSZ];

  memset(buf, 'a', BSZ - 2); /* fill some of the buffer */
  buf[BSZ-2] = '\0';
  buf[BSZ-1] = 'X';
  /* Open the memory stream; fmemopen() places null byte at beginning of buf */
  if ((fp = fmemopen(buf, BSZ, "w+")) == NULL) {
    err_sys("fmemopen failed");
  }
  printf("Initial buffer contents: %s\n", buf);
  fprintf(fp, "hello, world");
  printf("Before flush: %s\n", buf);
  fflush(fp);
  printf("After flush: %s\n", buf);
  printf("Length of string in buf = %ld\n", (long)strlen(buf));

  memset(buf, 'b', BSZ-2);
  buf[BSZ-2] = '\0';
  buf[BSZ-1] = 'X';
  fprintf(fp, "hello, world");
  fseek(fp, 0, SEEK_SET);
  printf("After fseek: %s\n", buf);
  printf("Length of string in buf = %ld\n", (long)strlen(buf));

  memset(buf, 'c', BSZ-2);
  buf[BSZ-2] = '\0';
  buf[BSZ-1] = 'X';
  fprintf(fp, "hello, world");
  fclose(fp);
  printf("After fclose: %s\n", buf);
  printf("Length of string in buf = %ld\n", (long)strlen(buf));

  return(0);
}