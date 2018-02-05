#include "opend.h"

/**
 * This function is called by buf_args(), which is called by handle_request().
 * The buf_args() function has broken up the client's buffer into an argv[]
 * style array, which this function processes.
 * @param argc argument count; number of elements in argv.
 * @param argv argument vector of strings.
 * @return 0 on success; -1 on error.
 */
int cli_args(int argc, char **argv) {
  if (argc != 3 || strcmp(argv[0], CL_OPEN) != 0) {
    strcpy(errmsg, "Usage: <pathname> <oflag>\n");
    return (-1);
  }
  pathname = argv[1]; /* save ptr to pathname to open */
  oflag = atoi(argv[2]);
  return (0);
}
