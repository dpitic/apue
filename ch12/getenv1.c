/*
 * This function demonstrates a possible implementation of the getenv()
 * function, which is not reentrant.  If two threads call it at the same time,
 * they will see inconsistent results, because the string returned is stored in
 * a single static buffer that is shared by all threads calling the function.
 */
#include <limits.h>
#include <string.h>

#define MAXSTRINGSZ 4096

static char envbuf[MAXSTRINGSZ];

extern char **environ;

char *getenv(const char *name) {
  int i, len;

  len = strlen(name);
  for (i = 0; environ[i] != NULL; i++) {
    if ((strncmp(name, environ[i], len) == 0) && (environ[i][len] == '=')) {
      strncpy(envbuf, &environ[i][len + 1], MAXSTRINGSZ - 1);
      return (envbuf);
    }
  }
  return (NULL);
}