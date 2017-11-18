/*
 * This function uses the standard interface to getenv() and implements a thread
 * safe version using thread specific data to maintain a per thread copy of the
 * data buffer used to hold the return string.  This implementation is not
 * async-signal safe because is calls malloc() which isn't async-signal safe.
 */
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MAXSTRINGSZ 4096

static pthread_key_t key;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;
pthread_mutex_t env_mutex = PTHREAD_MUTEX_INITIALIZER;

extern char **environ;

/* Thread initialisation function */
static void thread_init(void) {
  /*
   * The destructor is free(), used to free the environment buffer memory.  It
   * will only be called with the value of the thread specific data if the value
   * is not null.
   */
  pthread_key_create(&key, free);
}

char *getenv(const char *name) {
  int i, len;
  char *envbuf;

  /* Ensure only one key is created for the thread specific data */
  pthread_once(&init_done, thread_init);

  pthread_mutex_lock(&env_mutex);

  /* Check if the key has been allocated buffer memory previously*/
  envbuf = (char *)pthread_getspecific(key);
  if (envbuf == NULL) { /* No previously allocated buffer */
    envbuf = malloc(MAXSTRINGSZ);
    if (envbuf == NULL) { /* Can't allocate buffer memory */
      pthread_mutex_unlock(&env_mutex);
      return (NULL);
    }
    /* Assign thread specific key to the data buffer */
    pthread_setspecific(key, envbuf);
  }

  len = strlen(name);
  for (i = 0; environ[i] != NULL; i++) {
    if ((strncmp(name, environ[i], len) == 0) && (environ[i][len] == '=')) {
      strncpy(envbuf, &environ[i][len + 1], MAXSTRINGSZ - 1);
      pthread_mutex_unlock(&env_mutex);
      return (envbuf);
    }
  }
  pthread_mutex_unlock(&env_mutex);
  return (NULL); /* No matching environment variable found */
}