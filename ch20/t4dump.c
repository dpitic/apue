/*
 * Program used to dump the database to stdout.
 */
#include "apue.h"
#include "apue_db.h"
#include <fcntl.h>

int main(void) {
  DBHANDLE db;
  char *ptr = NULL;
  char key[IDXLEN_MAX];

  if ((db = db_open("db4", O_RDONLY, FILE_MODE)) == NULL) {
    err_sys("db_open() error");
  }

  /* db_rewind() must be called before db_nextrec() */
  db_rewind(db);
  while ((ptr = db_nextrec(db, key)) != NULL) {
    printf("%s\t%s\n", key, ptr);
  }

  db_close(db);
  exit(0);
}
