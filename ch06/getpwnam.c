/*
 * Implementation of the getpwnam() function used to return the password file
 * entry for the given login name.
 */
#include <pwd.h>
#include <stddef.h>
#include <string.h>

struct passwd *getpwnam(const char *name) {
  struct passwd *ptr;

  setpwent();   /* open passwd file and rewind to start of file */
  while ((ptr = getpwent()) != NULL) {  /* read next passwd record */
    if (strcmp(name, ptr->pw_name) == 0) {
      break;  /* found a match */
    }
  }
  endpwent();   /* close the passwd file */
  return(ptr);  /* ptr is NULL if no match found */
}
