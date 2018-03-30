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
  /* Read the file one line at a time, scanning for two strings separated by
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
