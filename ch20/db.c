#include "apue.h"
#include "apue_db.h"
#include <fcntl.h>    /* open() & db_open() flags */
#include <stdarg.h>
#include <errno.h>
#include <sys/uio.h>  /* struct iovec */

/*
 * Internal index file constants.  These are used to construct records in the
 * index file and data file.
 */
#define IDXLEN_SZ     4     /* index record length (ASCII characters) */
#define SEP         ':'     /* separator character in index record */
#define SPACE       ' '     /* space character */
#define NEWLINE    '\n'     /* newline character */

/*
 * The following definitions are for hash chains and free list chain in the
 * index file.
 */
#define PTR_SZ        7     /* size of ptr field in hash chain */
#define PTR_MAX 9999999     /* max file offset = 10**PTR_SZ - 1 */
#define  NHASH_DEF  137     /* default hash table size */
#define FREE_OFF      0     /* free list offset in index file */
#define HASH_OFF PTR_SZ     /* hash table offset in index file */

typedef unsigned long DBHASH;   /* hash values */
typedef unsigned long COUNT;    /* unsigned counter */

/*
 * Library private representation of the database.  Used to keep all the
 * information for each open database.  The DBHANDLE value that is returned by
 * db_open() and used by all the other functions is really just a pointer to
 * this DB structure.  Since pointer and lengths are stored as ASCII in the
 * database, these are converted to numeric values and saved in the DB struct.
 */
typedef struct {
  int     idxfd;          /* fd for index file */
  int     datafd;         /* fd for data file */
  char    *idxbuf;        /* malloc'ed buffer for index record */
  char    *databuf;       /* malloc'ed buffer for data record */
  char    *name;          /* name db was opened under */
  off_t   idxoff;         /* offset in index file of index record */
                          /* key is at (idxoff + PTR_SZ + IDXLEN_SZ) */
  size_t  idxlen;         /* length of index record */
                          /* excludes IDXLEN_SZ bytes at front of record */
                          /* includes newline at end of index record */
  off_t   dataoff;        /* offset in data file of data record */
  size_t  datalen;        /* length of data record */
                          /* includes newline at end */
  off_t   ptrval;         /* contents of chain ptr in index record */
  off_t   ptroff;         /* chain ptr offset pointing to this idx record */
  off_t   chainoff;       /* offset of hash chain for this index record */
  off_t   hashoff;        /* offset in index file of hash table */
  DBHASH  nhash;          /* current hash table size */

  /*
   * Counters for both successful and unsuccessful operations.  Useful for
   * analysing the performance of the database.
   */
  COUNT   cnt_delok;      /* delete OK */
  COUNT   cnt_delerr;     /* delete error */
  COUNT   cnt_fetchok;    /* fetch OK */
  COUNT   cnt_fetcherr;   /* fetch error */
  COUNT   cnt_nextrec;    /* nextrec */
  COUNT   cnt_stor1;      /* store: DB_INSERT, no empty, appended */
  COUNT   cnt_stor2;      /* store: DB_INSERT, found empty, reused */
  COUNT   cnt_stor3;      /* store: DB_REPLACE, diff len, appended */
  COUNT   cnt_stor4;      /* store: DB_REPLACE, same len, overwrote */
  COUNT   cnt_storerr;    /* store error */
} DB;

/*
 * Internal (private) functions.
 */
static DB     *_db_alloc(int);
static void   _db_dodelete(DB *);
static int    _db_find_and_lock(DB *, const char *, int);
static int    _db_findfree(DB *, int, int);
static void   _db_free(DB *);
static DBHASH _db_hash(DB *, const char *);
static char   *_db_readdat(DB *);
static off_t  _db_readidx(DB *, off_t);
static off_t  _db_readptr(DB *, off_t);
static void   _db_writedat(DB *, const char *, off_t, int);
static void   _db_writeidx(DB *, const char *, off_t, int, off_t);
static void   _db_writeptr(DB *, off_t, off_t);


