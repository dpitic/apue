#include "apue.h"
#include "apue_db.h"
#include <errno.h>
#include <fcntl.h> /* open() & db_open() flags */
#include <stdarg.h>
#include <sys/uio.h> /* struct iovec */

/*
 * Internal index file constants.  These are used to construct records in the
 * index file and data file.
 */
#define IDXLEN_SZ 4  /* index record length (ASCII characters) */
#define SEP ':'      /* separator character in index record */
#define SPACE ' '    /* space character */
#define NEWLINE '\n' /* newline character */

/*
 * The following definitions are for hash chains and free list chain in the
 * index file.
 */
#define PTR_SZ 7        /* size of ptr field in hash chain */
#define PTR_MAX 9999999 /* max file offset = 10**PTR_SZ - 1 */
#define NHASH_DEF 137   /* default hash table size */
#define FREE_OFF 0      /* free list offset in index file */
#define HASH_OFF PTR_SZ /* hash table offset in index file */

typedef unsigned long DBHASH; /* hash values */
typedef unsigned long COUNT;  /* unsigned counter */

/*
 * Library private representation of the database.  Used to keep all the
 * information for each open database.  The DBHANDLE value that is returned by
 * db_open() and used by all the other functions is really just a pointer to
 * this DB structure.  Since pointer and lengths are stored as ASCII in the
 * database, these are converted to numeric values and saved in the DB struct.
 */
typedef struct {
  int idxfd;      /* fd for index file */
  int datfd;      /* fd for data file */
  char *idxbuf;   /* malloc'ed buffer for index record */
  char *datbuf;   /* malloc'ed buffer for data record */
  char *name;     /* name db was opened under */
  off_t idxoff;   /* offset in index file of index record */
                  /* key is at (idxoff + PTR_SZ + IDXLEN_SZ) */
  size_t idxlen;  /* length of index record */
                  /* excludes IDXLEN_SZ bytes at front of record */
                  /* includes newline at end of index record */
  off_t datoff;   /* offset in data file of data record */
  size_t datlen;  /* length of data record */
                  /* includes newline at end */
  off_t ptrval;   /* contents of chain ptr in index record */
  off_t ptroff;   /* chain ptr offset pointing to this idx record */
  off_t chainoff; /* offset of hash chain for this index record */
  off_t hashoff;  /* offset in index file of hash table */
  DBHASH nhash;   /* current hash table size */

  /*
   * Counters for both successful and unsuccessful operations.  Useful for
   * analysing the performance of the database.
   */
  COUNT cnt_delok;    /* delete OK */
  COUNT cnt_delerr;   /* delete error */
  COUNT cnt_fetchok;  /* fetch OK */
  COUNT cnt_fetcherr; /* fetch error */
  COUNT cnt_nextrec;  /* nextrec */
  COUNT cnt_stor1;    /* store: DB_INSERT, no empty, appended */
  COUNT cnt_stor2;    /* store: DB_INSERT, found empty, reused */
  COUNT cnt_stor3;    /* store: DB_REPLACE, diff len, appended */
  COUNT cnt_stor4;    /* store: DB_REPLACE, same len, overwrote */
  COUNT cnt_storerr;  /* store error */
} DB;

/*
 * Internal (private) functions; prefixed with _db_
 */
static DB *_db_alloc(int);
static void _db_dodelete(DB *);
static int _db_find_and_lock(DB *, const char *, int);
static int _db_findfree(DB *, int, int);
static void _db_free(DB *);
static DBHASH _db_hash(DB *, const char *);
static char *_db_readdat(DB *);
static off_t _db_readidx(DB *, off_t);
static off_t _db_readptr(DB *, off_t);
static void _db_writedat(DB *, const char *, off_t, int);
static void _db_writeidx(DB *, const char *, off_t, int, off_t);
static void _db_writeptr(DB *, off_t, off_t);

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
  DB *db;
  int mode;
  size_t len, i;
  char asciiptr[PTR_SZ + 1],
      hash[(NHASH_DEF + 1) * PTR_SZ + 2]; /* +2 for newline & null */
  struct stat statbuff;

  /*
   * Allocate a DB structure, and the buffers it needs.
   */
  len = strlen(pathname);
  if ((db = _db_alloc(len)) == NULL) {
    err_dump("db_open(): _db_alloc() error for DB");
  }
  db->nhash = NHASH_DEF;  /* hash table size */
  db->hashoff = HASH_OFF; /* offset in index file of hash table */
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
  db->idxfd = db->datfd = -1; /* descriptors */

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
  return (db);
} /* _db_alloc() */

/**
 * Relinquish access to the database.  This function closes the index file and
 * the data file and releases any memory that it allocated for internal buffers.
 * @param h database handle.
 */
void db_close(DBHANDLE h) {
  _db_free((DB *)h); /* closes fds, free buffers & struct */
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
    ptr = NULL; /* error, record not found */
    db->cnt_fetcherr++;
  } else {
    ptr = _db_readdat(db); /* return pointer to data */
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
      break; /* match found */
      /*
       * ptroff contains address of previous index record
       * datoff contains address of the data record
       * datlen contains the size of the data record
       */
    }
    db->ptroff = offset; /* offset of this (unequal) record */
    offset = nextoffset; /* next one to compare; 0 == end of hash chain */
  }

  /*
   * offset == 0 on error (record not found).
   */
  return (offset == 0 ? -1 : 0);
} /* _db_find_and_lock() */

/**
 * Calculate the hash value for a key.
 * @param db pointer to database structure.
 * @param key pointer to key string.
 * @return hash value for the given key.
 */
static DBHASH _db_hash(DB *db, const char *key) {
  DBHASH hval = 0;
  char c;
  int i;

  /*
   * Hash value for a key is calculated by multiplying each ASCII character by
   * its 1-based index and dividing the result by the number of hash table
   * entries, with the remainder of the division being the hash value.
   */
  for (i = 1; (c = *key++) != 0; i++) {
    hval += c * i; /* ascii char times its 1-based index */
  }
  return (hval % db->nhash);
} /* _db_hash() */

/**
 * Read a chain ptr field from anywhere in the index file: the free list
 * pointer, a hash table chain ptr, or an index record chain ptr.  This function
 * does not perform any locking of the database files.
 * @param db pointer to database structure.
 * @param offset pointer offset to start reading from.
 * @return pointer read at the given offset, on success; core dump on failure.
 */
static off_t _db_readptr(DB *db, off_t offset) {
  char asciiptr[PTR_SZ + 1];

  if (lseek(db->idxfd, offset, SEEK_SET) == -1) {
    err_dump("_db_readptr(): lseek() error to ptr field");
  }
  if (read(db->idxfd, asciiptr, PTR_SZ) != PTR_SZ) {
    err_dump("_db_readptr(): read() error of ptr field");
  }
  asciiptr[PTR_SZ] = 0; /* null terminate */
  return (atol(asciiptr));
} /* _db_readptr() */

/**
 * Read the next index record, starting at the specified offset in the index
 * file.  Read the index record into db->idxbuf and replace the separators with
 * null bytes.  On success, set db->datoff and db->datlen to the offset and
 * length of the corresponding data record in the data file.
 * @param db pointer to database structure.
 * @param offset in the index file.
 * @return
 */
static off_t _db_readidx(DB *db, off_t offset) {
  ssize_t i;
  char *ptr1, *ptr2;
  char asciiptr[PTR_SZ + 1], asciilen[IDXLEN_SZ + 1];
  struct iovec iov[2];

  /*
   * Position index file and record the offset.  db_nextkey() calls this
   * function with offet == 0, meaning read from current offset.  Still need to
   * call lseek() to record the current offset.  Since an index record will
   * never be stored at offset 0 in the index file, the offset value 0 can be
   * safely overloaded to mean - read from the current offset.
   */
  if ((db->idxoff =
           lseek(db->idxfd, offset, offset == 0 ? SEEK_CUR : SEEK_SET)) == -1) {
    err_dump("_db_readidx(): lseek() error");
  }

  /*
   * Read the ascii chain ptr and the ascii length at the front of the index
   * record.  This provides the remaining size of the index record.
   */
  iov[0].iov_base = asciiptr;
  iov[0].iov_len = PTR_SZ;
  iov[1].iov_base = asciilen;
  iov[1].iov_len = IDXLEN_SZ;
  if ((i = readv(db->idxfd, &iov[0], 2)) != PTR_SZ + IDXLEN_SZ) {
    if (i == 0 && offset == 0) {
      return (-1); /* EOF for db_nextkey() */
    }
    err_dump("_db_readidx(): readv() error of index record");
  }

  /*
   * Return value; always >= 0
   */
  asciiptr[PTR_SZ] = 0;        /* null terminate */
  db->ptrval = atol(asciiptr); /* offset of next key in chain */

  asciilen[IDXLEN_SZ] = 0; /* null terminate */
  if ((db->idxlen = atoi(asciilen)) < IDXLEN_MIN || db->idxlen > IDXLEN_MAX) {
    err_dump("_db_readidx(): invalid length");
  }

  /*
   * Now read the actual index record.  Read it into the key buffer that was
   * malloced when the database was opened.
   */
  if ((i = read(db->idxfd, db->idxbuf, db->idxlen)) != db->idxlen) {
    err_dump("_db_readidx(): read() error of index record");
  }
  if (db->idxbuf[db->idxlen - 1] != NEWLINE) { /* sanity check */
    err_dump("_db_readidx(): missing newline");
  }
  db->idxbuf[db->idxlen - 1] = 0; /* replace newline with null */

  /*
   * Find the separators in the index record.  Separate the index record into
   * three fields:
   *   1. key
   *   2. offset of the corresponding data record.
   *   3. length of the data record.
   * The strchr() function finds the first occurrence of the specified character
   * in the given string.  Here we look for the character that separates fields
   * in the record (SEP, which is defined to be a colon).
   */
  if ((ptr1 = strchr(db->idxbuf, SEP)) == NULL) {
    err_dump("_db_readidx(): missing first separator");
  }
  *ptr1++ = 0; /* replace SEP with null */

  if ((ptr2 = strchr(ptr1, SEP)) == NULL) {
    err_dump("_db_readidx(): missing second separator");
  }
  *ptr2++ = 0; /* replace SEP with null */

  if (strchr(ptr2, SEP) != NULL) {
    err_dump("_db_readidx(): too many separators");
  }

  /*
   * Get the starting offset and length of the data record.
   */
  if ((db->datoff = atol(ptr1)) < 0) {
    err_dump("_db_readidx(): starting offset < 0");
  }
  if ((db->datlen = atol(ptr2)) <= 0 || db->datlen > DATLEN_MAX) {
    err_dump("_db_readidx(): invalid length");
  }
  return (db->ptrval); /* return offset of next key in chain */
} /* _db_readidx() */

/**
 * Read the current data record into the data buffer.  Return a pointer to the
 * null-terminated data buffer.
 * @param db pointer to database structure.
 * @return pointer to the null-terminated data buffer.
 */
static char *_db_readdat(DB *db) {
  if (lseek(db->datfd, db->datoff, SEEK_SET) == -1) {
    err_dump("_db_readdat(): lseek() error");
  }
  if (read(db->datfd, db->datbuf, db->datlen) != db->datlen) {
    err_dump("_db_readdat(): read() error");
  }
  if (db->datbuf[db->datlen - 1] != NEWLINE) { /* sanity check */
    err_dump("_db_readdat(): missing newline");
  }
  db->datbuf[db->datlen - 1] = 0; /* replace newline with null */
  return (db->datbuf);            /* return pointer to data record */
} /* _db_readdat() */
