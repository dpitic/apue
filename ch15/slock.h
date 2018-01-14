#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <sys/stat.h>

struct slock {
  sem_t *semp;
  char name[_POSIX_NAME_MAX];
};

struct slock *s_alloc();
void s_free(struct slock *);
int s_lock(struct slock *);
int s_unlock(struct slock *);
