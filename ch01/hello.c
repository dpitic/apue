/*
 * This program prints its process ID (PID).
 */
#include "apue.h"

int main(void) {
  printf("hello world from process ID %ld\n", (long)getpid());
  exit(0);
}