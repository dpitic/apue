/*
 * The main() function of the open server, opend.  It reads requests from the
 * client on the fd-pipe (its standard input) and calls the function
 * handle_request(), which implements the open server request logic.
 */
#include "opend.h"

char errmsg[MAXLINE];
int oflag;
char *pathname;

int main(void) {
  int nread;
  char buf[MAXLINE];

  for (;;) { /* read arg buffer from client, proess request */
    if ((nread = read(STDIN_FILENO, buf, MAXLINE)) < 0) {
      err_sys("read() error on stream pipe");
    } else if (nread == 0) {
      break; /* client has closed the stream pipe */
    }
    handle_request(buf, nread, STDOUT_FILENO);
  }
  exit(0);
}
