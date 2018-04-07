/*
 * The client command for printing documents.  Opens the file and sends it to
 * the printer spooling daemon.  Usage:
 *   $ print [-t] filename
 */
#include "print.h"
#include "apue.h"
#include <fcntl.h>
#include <pwd.h>

/**
 * Flag used to control the log functions in the library.  If set to a nonzero
 * value, error messages will be sent to the standard error stream instead of
 * to a log file.
 */
int log_to_stderr = 1;

void submit_file(int, int, const char *, size_t, int);

int main(int argc, char *argv[]) {
  int fd, sfd, err, text, c;
  struct stat sbuf;
  char *host;
  struct addrinfo *ailist, *aip;

  err = 0;
  text = 0;
  while ((c = getopt(argc, argv, "t")) != -1) {
    switch (c) {
    case 't': /* print file as text (instead of as PostScript) */
      text = 1;
      break;
    case '?':
      err = 1;
      break;
    }
  }

  /* Input error processing */
  if (err || (optind != argc - 1)) {
    err_quit("Usage: %s [-t] filename", argv[0]);
  }
  if ((fd = open(argv[optind], O_RDONLY)) < 0) {
    err_sys("print: can't open %s", argv[optind]);
  }
  if (fstat(fd, &sbuf) < 0) {
    err_sys("print: can't stat %s", argv[optind]);
  }
  if (!S_ISREG(sbuf.st_mode)) {
    err_quit("print: %s must be a regular file", argv[optind]);
  }

  /*
   * Get the hostname of the host acting as the print server.
   */
  if ((host = get_printserver()) == NULL) {
    err_quit("print: no print server defined");
  }
  /*
   * Translate host name into a network address; service "print" must be in
   * /etc/services file.
   */
  if ((err = getaddrlist(host, "print", &ailist)) != 0) {
    err_quit("print: getaddrinfo() error: %s", gai_strerror(err));
  }
  /*
   * Try to connect to the daemon using one address at a time from the list
   * returned by getaddrinfo().  Send the file to the daemon using the first
   * address to which a connection can be made.
   */
  for (aip = ailist; aip != NULL; aip = aip->ai_next) {
    if ((sfd = connect_retry(AF_INET, SOCK_STREAM, 0, aip->ai_addr,
                             aip->ai_addrlen)) < 0) {
      err = errno;
    } else {
      submit_file(fd, sfd, argv[optind], sbuf.st_size, text);
      exit(0);
    }
  }
  err_exit(err, "print: can't contact %s", host);
} /* main() */

/**
 * Send a file to the print spooler daemon.
 * @param fd file descriptor of file to print.
 * @param sockfd socket file descriptor for communication with print server.
 * @param fname pointer to null-terminated string for file name to print.
 * @param nbytes size of file in bytes.
 * @param text control flag to indicate text file (rather than PostScript).
 */
void submit_file(int fd, int sockfd, const char *fname, size_t nbytes,
                 int text) {
  int nr, nw, len;
  struct passwd *pwd;
  struct printreq req;
  struct printresp res;
  char buf[IOBUFSZ];

  /* Build header */
  if ((pwd = getpwuid(getuid())) == NULL) {
    strcpy(req.usernm, "unknown");
  } else {
    strncpy(req.usernm, pwd->pw_name, USERNM_MAX - 1);
    req.usernm[USERNM_MAX - 1] = '\0';
  }
  /* Convert file size to network byte order and save in header */
  req.size = htonl(nbytes);

  if (text) {
    /* Convert plain text flag to network byte order and save in header flags */
    req.flags = htonl(PR_TEXT);
  } else {
    req.flags = 0;
  }

  /* Set job name to the name of file being printed */
  if ((len = strlen(fname)) >= JOBNM_MAX) {
    /*
     * Truncate the filename (+-5 accounts for the leading four characters and
     * the terminating null).
     */
    strcpy(req.jobnm, "... "); /* prepend ellipsis to job name */
    strncat(req.jobnm, &fname[len - JOBNM_MAX + 5], JOBNM_MAX - 5);
  } else {
    strcpy(req.jobnm, fname);
  }

  /*
   * Send the header to the print server.
   */
  nw = writen(sockfd, &req, sizeof(struct printreq));
  if (nw != sizeof(struct printreq)) {
    if (nw < 0) {
      err_sys("Can't write to print server");
    } else {
      err_quit("Short write (%d/%d) to print server", nw,
               sizeof(struct printreq));
    }
  }

  /*
   * Send file to print server.
   */
  while ((nr = read(fd, buf, IOBUFSZ)) != 0) {
    nw = writen(sockfd, buf, nr);
    if (nw != nr) {
      if (nw < 0) {
        err_sys("Can't write to print server");
      } else {
        err_quit("Short write (%d/%d) to print server", nw, nr);
      }
    }
  }

  /*
   * Read response from print server.
   */
  if ((nr = readn(sockfd, &res, sizeof(struct printresp))) !=
      sizeof(struct printresp)) {
    err_sys("Can't read response from server");
  }
  if (res.retcode != 0) {
    printf("rejected: %s\n", res.msg);
    exit(1);
  } else {
    printf("job ID %ld\n", (long)ntohl(res.jobid));
  }
} /* submit_file() */
