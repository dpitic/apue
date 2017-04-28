/*
 * This program changes to a specific directory and then calls the getcwd()
 * function to print the working directory.
 */
#include "apue.h"

int main(int argc, char const *argv[]) {
  char *ptr;
  size_t size;

  if (chdir("/var/spool/") < 0) {
    err_sys("chdir failed");
  }

  ptr = path_alloc(&size); /* custom path allocation function */
  if (getcwd(ptr, size) == NULL) {
    err_sys("getcwd failed");
  }

  printf("cwd = %s\n", ptr);
  exit(0);
}
