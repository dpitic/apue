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
#define NHASH_DEF   137     /* default hash table size */
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
  int     datfd;          /* fd for data file */
  char    *idxbuf;        /* malloc'ed buffer for index record */
  char    *datbuf;        /* malloc'ed buffer for data record */
  char    *name;          /* name db was opened under */
  off_t   idxoff;         /* offset in index file of index record */
                          /* key is at (idxoff + PTR_SZ + IDXLEN_SZ) */
  size_t  idxlen;         /* length of index record */
                          /* excludes IDXLEN_SZ bytes at front of record */
                          /* includes newline at end of index record */
  off_t   datoff;         /* offset in data file of data record */
  size_t  datlen;         /* length of data record */
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
 * Internal (private) functions; prefixed with _db_
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

/**
 * Open or create a database.  Same arguments as open(2).  If successful, two
 * files are created:
 *   1. pathname.idx - index file; initialised if necessary.
 *   2. pathname.dat - data file.
 * @param pathname string containing prefix of database filenames.
 * @param oflag used as the 2nd argument to open(2), to specify how the files
 * are to be opened e.g. read-only, read-write, create file if it doesn't exit.
 * @param ... int mode used as 3rd argument to open(2) for file access
 * permissions, if the database files are created.
 * @return handle (opaque pointer) representing the database if OK; NULL on 
 * error.
 */
DBHANDLE db_open(const char *pathname, int oflag, ...) {
  DB          *db;
  int         mode;
  size_t      len, i;
  char        asciiptr[PTR_SZ + 1],
              hash[(NHASH_DEF + 1) * PTR_SZ + 2];   /* +2 for newline & null */
  struct stat statbuff;

  /*
   * Allocate a DB structure, and the buffers it needs.
   */
  len = strlen(pathname);
  if ((db = _db_alloc(len)) == NULL) {
    err_dump("db_open(): _db_alloc() error for DB");
  }
  db->nhash = NHASH_DEF;    /* hash table size */
  db->hashoff = HASH_OFF;   /* offset in index file of hash table */
  strcpy(db->name, pathname);
  strcat(db->name, ".idx");

  /* Check if the caller wants to create the database files */
  if (oflag & O_CREAT) {
    va_list ap;

    va_start(ap, oflag);
    mode = va_arg(ap, int);
    va_end(ap);

    /*
     * Open index file and data file.
     */
    db->idxfd = open(db->name, oflag, mode);
    strcpy(db->name + len, ".dat");
    db->datfd = open(db->name, oflag, mode);
  } else {
    /*
     * Open existing index file and data file.
     */
    db->idxfd = open(db->name, oflag);
    strcpy(db->name + len, ".dat");
    db->datfd = open(db->name, oflag);
  }

  /* 
   * Check if any errors occurred while opening or creating either database
   * file.
   */
  if (db->idxfd < 0 || db->datfd < 0) {
    _db_free(db);
    return (NULL);
  }

  if ((oflag & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC)) {
    /*
     * If the database was created, we have to initialise it.  Write lock the
     * entire file so that we can stat it, check its size, and initialise it,
     * in an atomic operation.
     */
    if (writew_lock(db->idxfd, 0, SEEK_SET, 0) < 0) {
      err_dump("db_open(): writew_lock() error");
    }

    if (fstat(db->idxfd, &statbuff) < 0) {
      err_sys("db_open(): fstat() error");
    }

    if (statbuff.st_size == 0) {
      /*
       * We have to build a list of (NHASH_DEF + 1) chain ptrs with a value of
       * 0.  The +1 is for the free list pointer that precedes the hash table.
       */
      sprintf(asciiptr, "%*d", PTR_SZ, 0);
      hash[0] = 0;
      for (i = 0; i < NHASH_DEF + 1; i++) {
        strcat(hash, asciiptr);
      }
      strcat(hash, "\n");
      i = strlen(hash);
      if (write(db->idxfd, hash, i) != i) {
        err_dump("db_open(): index file init write() error");
      }
    }
    if (un_lock(db->idxfd, 0, SEEK_SET, 0) < 0) {
      err_dump("db_open(): un_lock() error");
    }
  }
  db_rewind(db);
  return (db);
} /* db_open() */

/**
 * Allocate and initialise a DB structure and its buffers.
 * @param namelen length of database name string (without extension).
 * @return pointer to DB structure on success; dump core on error.
 */
static DB *_db_alloc(int namelen) {
  DB *db;

  /*
   * Use calloc() to initialise the structure to zero.
   */
  if ((db = calloc(1, sizeof(DB))) == NULL) {
    err_dump("_db_alloc(): calloc() error for DB");
  }
  /*
   * Side effect of calloc() sets database file descriptors to 0; reset fd to -1
   * to indicate that they are not yet valid.
   */
  db->idxfd = db->datfd = -1;		/* descriptors */

  /*
   * Allocate room for the name.
   * +5 for ".idx" or ".dat" plus null at end.
   */
  if ((db->name = malloc(namelen + 5)) == NULL) {
    err_dump("_db_alloc(): malloc() error for name");
  }

  /*
   * Allocate an index buffer and a data buffer.
   * +2 for newline and null at end.
   */
  if ((db->idxbuf = malloc(IDXLEN_MAX + 2)) == NULL) {
    err_dump("_db_alloc(): malloc() error for index buffer");
  }
  if ((db->datbuf = malloc(DATLEN_MAX + 2)) == NULL) {
    err_dump("_db_alloc(): malloc() error for data buffer");
  }
  return(db);
} /* _db_alloc() */

/**
 * Relinquish access to the database.  This function closes the index file and
 * the data file and releases any memory that it allocated for internal buffers.
 * @param h database handle.
 */
void db_close(DBHANDLE h) {
  _db_free((DB *)h);		/* closes fds, free buffers & struct */
}

/**
 * Free up a DB structure, and all the malloc'ed buffers it may point to.  Also
 * close the file descriptors if still open.
 * @param db pointer to DB structure.
 */
static void _db_free(DB *db) {
  if (db->idxfd >= 0) {
    close(db->idxfd);
  }
  if (db->datfd >= 0) {
    close(db->datfd);
  }
  if (db->idxbuf != NULL) {
    free(db->idxbuf);
  }
  if (db->datbuf != NULL) {
    free(db->datbuf);
  }
  if (db->name != NULL) {
    free(db->name);
  }
  free(db);
}

/**
 * Fetch a record and return a pointer to the null-terminated data.
 * @param h database handle.
 * @param key lookup key for the data record.
 * @return pointer to the data stored with key, if the record is found; NULL if
 * the record is not found.
 */
char *db_fetch(DBHANDLE h, const char *key) {
  DB *db = h;
  char *ptr;

  if (_db_find_and_lock(db, key, 0) < 0) {
    ptr = NULL;               /* error, record not found */
    db->cnt_fetcherr++;
  } else {
    ptr = _db_readdat(db);    /* return pointer to data */
    db->cnt_fetchok++;
  }

  /*
   * Unlock the hash chain that _db_find_and_lock() locked.
   */
  if (un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0) {
    err_dump("db_fetch(): un_lock() error");
  }
  return (ptr);
} /* db_fetch() */

/**
 * Find the specified record.  Called by db_delete(), db_fetch(), and
 * db_store().  Returns with the hash chain locked.
 * @param db pointer to database object.
 * @param key search key.
 * @param writelock nonzero value to acquire a write lock on the index file
 * while searching for the record.
 * @return 0 if record found; -1 if record not found.
 */
static int _db_find_and_lock(DB *db, const char *key, int writelock) {
  off_t offset, nextoffset;

  /*
   * Calculate the hash value for this key, then calculate the byte offset of
   * corresponding chain ptr in hash table.  This is where the search starts.
   * First calculate the offset in the hash table for this key.
   */
  db->chainoff = (_db_hash(db, key) * PTR_SZ) + db->hashoff;
  db->ptroff = db->chainoff;

  /*
   * Lock the hash chain here.  The caller must unlock it when done.  Note,
   * only lock and unlock the first byte.  This increases concurrency by
   * allowing multiple processes to search different hash chains at the same
   * time.
   */
  if (writelock) {
    if (writew_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0) {
      err_dump("_db_find_and_lock(): writew_lock() error");
    }
  } else {
    /* Read lock the index file while searching it */
    if (readw_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0) {
      err_dump("_db_find_and_lock(): readw_lock() error");
    }
  }

  /*
   * Get the offset in the index file of first record on the hash chain
   * (can be 0 if the hash chain is empty).
   */
  offset = _db_readptr(db, db->ptroff);
  /* Loop through each index record on the hash chain, comparing keys */
  while (offset != 0) {
    /*
     * Read each index record, populating idxbuf field with key of current
     * record.  _db_readidx() returns 0 when the last entry in the chain is
     * reached.
     */
    nextoffset = _db_readidx(db, offset);
    if (strcmp(db->idxbuf, key) == 0) {
      break;    /* match found */
      /*
       * ptroff contains address of previous index record
       * datoff contains address of the data record
       * datlen contains the size of the data record
       */
    }
    db->ptroff = offset;    /* offset of this (unequal) record */
    offset = nextoffset;    /* next one to compare; 0 == end of hash chain */
  }

  /*
   * offset == 0 on error (record not found).
   */
  return (offset == 0 ? -1 : 0);
} /* _db_find_and_lock() */