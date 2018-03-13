/*
 * Database library header file.  This DB library is similar to the ndbm
 * library, but with the additional support for concurrency control mechanisms
 * to allow multiple processes to update the same database at the same time.
 */
#ifndef _APUE_DB_H
#define _APUE_DB_H

/**
 * Opaque pointer representing the database, that gets returned when a database
 * is opened.  This handle gets passed to the remaining database functions.
 */
typedef void* DBHANDLE;

/*
 * Function prototypes for database library public functions.
 */
DBHANDLE db_open(const char *, int, ...);
void db_close(DBHANDLE);

int db_store(DBHANDLE, const char *, const char *, int);
char *db_fetch(DBHANDLE, const char *);
int db_delete(DBHANDLE, const char *);

void db_rewind(DBHANDLE);
char *db_nextkey(DBHANDLE, char *);

/*
 * Flags for db_store()
 */
#define DB_INSERT   1   /* insert a new record only */
#define DB_REPLACE  2   /* replace existing record */
#define DB_STORE    3   /* replace or insert */

/*
 * Implementation limits
 */
#define IDXLEN_MIN  6       /* key, sep, start, sep, length, \n */
#define IDXLEN_MAX  1024    /* arbitrary */
#define DATLEN_MIN  2       /* data byte, newline */
#define DATLEN_MAX  1024    /* arbitrary */

#endif /* _APUE_DB_H */
