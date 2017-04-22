/*
 * This program outputs the user's ID (uid) and group ID (gid) to stdout.
 */
#include "apue.h"

int main(void) {
  printf("uid = %d, gid = %d\n", getuid(), getgid());
  exit(0);
}