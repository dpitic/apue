#include "apue.h"
#include "print.h"
#include <ctype.h>
#include <sys/select.h>

/**
 * Maximum size of a line in the printer configuration file.
 */
#define MAXCFGLINE 512
/**
 * Maximum size of a keyword in the configuration file.
 */
#define MAXKWLEN 16
/**
 * Maximum size of the format string passed to sscanf().
 */
#define MAXFMTLEN 16

/**
 * This function is a wrapper for getaddrinfo(), since the implementation
 * always calls getaddrinfo() with the same hint structure.  The function is
 * used to get the address list for the given host and service, which is
 * returned through ailistpp.  Returns 0 on success or an error code on failure.
 * Note that errno is not set if an error is encountered.  This function does
 * not need or perform any locking.
 * @param host pointer to null-terminated string containing hostname.
 * @param service pointer to null-terminated string containing name of service.
 * @param ailistpp pointer to a linked list of one or more addrinfo structs.
 * The elements in the linked list are linked by the ai_next field.
 * @return 0 on success; error code on failure.
 */
int getaddrlist(const char *host, const char *service,
                struct addrinfo **ailistpp) {
  int err;
  struct addrinfo hint;

  /* Initialise the hints argument for getaddrinfo() */
  hint.ai_flags = AI_CANONNAME;
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = 0; /* any protocol */
  hint.ai_addrlen = 0;
  hint.ai_canonname = NULL;
  hint.ai_addr = NULL;
  hint.ai_next = NULL;
  err = getaddrinfo(host, service, &hint, ailistpp);
  return (err);
}

/**
 * Given a keyword, scan the configuration file for a match and return the
 * string value corresponding to the keyword.  This function is not thread safe.
 * @param keyword string containing the keyword to search for.
 * @return string containing the value for the corresponding keyword if found;
 * NULL if keyword not found.
 */
static char *scan_configfile(char *keyword) {
  int n, match;
  FILE *fp;
  char keybuf[MAXKWLEN], pattern[MAXFMTLEN];
  char line[MAXCFGLINE];
  static char valbuf[MAXCFGLINE];

  if ((fp = fopen(CONFIG_FILE, "r")) == NULL) {
    log_sys("Can't open %s", CONFIG_FILE);
  }
  /*
   * Build the format string corresponding to the search pattern.  The %%%ds
   * builds a format specifier that limits the string so that the buffer used
   * to store the string on the stack is not overrun.
   */
  sprintf(pattern, "%%%ds %%%ds", MAXKWLEN - 1, MAXCFGLINE - 1);
  match = 0;
  /*
   * Read the file one line at a time, scanning for two strings separated by
   * white space.
   */
  while (fgets(line, MAXCFGLINE, fp) != NULL) {
    n = sscanf(line, pattern, keybuf, valbuf);
    /* Found a key-value; check keybuf against keyword */
    if (n == 2 && strcmp(keyword, keybuf) == 0) {
      match = 1; /* match found; value is in valbuf */
      break;
    }
  }
  fclose(fp);
  if (match != 0) {
    return (valbuf);
  } else {
    return (NULL);
  }
}

/**
 * Return the host name running the print server, or NULL on error.  This is
 * simply a wrapper function that calls scan_configfile() to find the name of
 * the computer system where the sprinter spooling daemon is running.
 * @return host name running the print server on success, or NULL on error.
 */
char *get_printserver(void) { return (scan_configfile("printserver")); }

/**
 * Return the address of the network printer, or NULL on error.
 * @return address of the network printer on success, or NULL on error.
 */
struct addrinfo *get_printaddr(void) {
  int err;
  char *p;
  struct addrinfo *ailist;

  if ((p = scan_configfile("printer")) != NULL) {
    if ((err = getaddrlist(p, "ipp", &ailist)) != 0) {
      log_msg("No address information for %s", p);
      return (NULL);
    }
    return (ailist);
  }
  log_msg("No printer address specified");
  return (NULL);
}

/**
 * "Timed" read - timeout specifies the number of seconds to wait before giving
 * up.  This function is suitable for preventing DOS attacks on the printer
 * spooling daemon.  Returns the number of bytes read, or -1 on error.
 * @param fd file descriptor to read from.
 * @param buf pointer to buffer used to read data into.
 * @param nbytes size of buf.
 * @param timeout number of seconds to wait before aborting read attempt.
 * @return number of bytes read on success, which can be less than requested if
 * all the data doesn't arrive in time; -1 on error, with errno set to
 * ETIME if data is not received before the specified timeout.
 */
ssize_t tread(int fd, void *buf, size_t nbytes, unsigned int timeout) {
  int nfds;
  fd_set readfds;
  struct timeval tv;

  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  /* Wait for the specified file descriptor to be readable */
  nfds = select(fd + 1, &readfds, NULL, NULL, &tv);
  if (nfds <= 0) {
    if (nfds == 0) {
      errno = ETIME;
    }
    return (-1);
  }
  return (read(fd, buf, nbytes));
}

/**
 * "Timed" read - timeout specifies the number of seconds to wait per read()
 * call before giving up, but read exactly nbytes.  Returns number of bytes
 * read or -1 on error.
 * @param fd file descriptor to read from.
 * @param buf pointer to buffer used to read data into.
 * @param nbytes number of bytes to read.
 * @param timeout number of seconds to wait before aborting the read attempt.
 * @return number of bytes read on success; -1 on error.
 */
ssize_t treadn(int fd, void *buf, size_t nbytes, unsigned int timeout) {
  size_t nleft;  /* number of bytes left to read */
  ssize_t nread; /* number of bytes read */

  nleft = nbytes;
  while (nleft > 0) {
    if ((nread = tread(fd, buf, nleft, timeout)) < 0) {
      if (nleft == nbytes) {
        return (-1); /* error; return -1 */
      } else {
        break; /* error; return amount read so far */
      }
    } else if (nread == 0) {
      break; /* EOF */
    }
    nleft -= nread;
    buf += nread;
  }
  return (nbytes - nleft); /* return >= 0 */
}
