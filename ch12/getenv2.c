/*
 * This function is a reentrant version of getevn().  It uses the pthread_once()
 * function to ensure that the thread_init() function is called only once per
 * process, regardless of how many threads might race to call getenv_r() at the
 * same time.
 */
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;

pthread_mutex_t env_mutex;

static pthread_once_t init_done = PTHREAD_ONCE_INIT;

/* Thread initialisation function */
static void thread_init(void) {
  pthread_mutexattr_t attr;

  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&env_mutex, &attr);
  pthread_mutexattr_destroy(&attr);
}
/*
 * Reentrant version of the getenv() function.  Made possible by requiring the
 * caller to provide the buffer so that each thread can use a different buffer
 * to avoid interference.  This function is not thread safe because there is
 * no protection against changes to the environment while searching for the 
 * requested string.
 */
int getenv_r(const char *name, char *buf, int buflen) {
  int i, len, olen;
  pthread_once(&init_done, thread_init);
  len = strlen(name);
  pthread_mutex_lock(&env_mutex);
  for (i = 0; environ[i] != NULL; i++) {
    if ((strncmp(name, environ[i], len) == 0) && (environ[i][len] == '=')) {
      olen = strlen(&environ[i][len + 1]);
      if (olen >= buflen) {
        pthread_mutex_unlock(&env_mutex);
        return (ENOSPC);
      }
      strcpy(buf, &environ[i][len + 1]);
      pthread_mutex_unlock(&env_mutex);
    }
  }
  pthread_mutex_unlock(&env_mutex);
  return (ENOENT);
}