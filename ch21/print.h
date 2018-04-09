#ifndef _PRINT_H
#define _PRINT_H

/*
 * Print server header file.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>

/**
 * Printer configuration file contains the host names of the printer spooling
 * daemon and the network attached printer.
 */
#define CONFIG_FILE "/tmp/etc/printer.conf"
/**
 * Directories used by the implementation must be created by an administrator
 * and be owned by the same user account under which the printer spooling daemon
 * runs.  The daemon won't try to create these directories if they don't exits,
 * because the daemon would need root privileges to create directories in
 * /var/spool.  The daemon is designed to do as little as possible while running
 * as root to minimise the chance of creating a security hole.
 */
#define SPOOLDIR "/tmp/var/spool/printer"
/**
 * Directory that holds file containing next job number; appended to SPOOLDIR.
 */
#define JOBFILE "jobno"
/**
 * Directory that holds copies of files to be printed; appended to SPOOLDIR.
 */
#define DATADIR "data"
/**
 * Directory that holds control information for each request; appended to
 * SPOOLDIR.
 */
#define REQDIR "reqs"

/* Define account name under which printer spooling daemon will run */
#if defined(BSD) || defined(LINUX)
/*
 * BSD and Linux doesn't define a separate account for the printer spooling
 * daemon, therefore use the account reserved for system daemons.
 */
#define LPNAME "daemon"
#elif defined(MACOS)
#define LPNAME "_lp"
#else
#define LPNAME "lp"
#endif

/**
 * File name size limit.
 */
#define FILENMSZ 64
/**
 * File permissions used when creating copies of files submitted to by printed.
 */
#define FILEPERM (S_IRUSR | S_IWUSR)

#define USERNM_MAX 64
#define JOBNM_MAX 256
#define MSGLEN_MAX 512

#ifndef HOST_NAME_MAX
/* If unable to determine system limit with sysconf() */
#define HOST_NAME_MAX 256
#endif

/**
 * IPP is defined to use port 631.
 */
#define IPP_PORT 631
/**
 * Backlog parameter passed to listen().
 */
#define QLEN 10

/**
 * IPP header buffer size.
 */
#define IBUFSZ 512
/**
 * HTTP header buffer size.
 */
#define HBUFSZ 512
/**
 * Data buffer size.
 */
#define IOBUFSZ 8192

/**
 * Some platforms don't define the error ETIME, so it is defined to an alternate
 * error code that makes sense for these systems.  This error code is returned
 * when a read times out (so that the server doesn't block indefinitely reading
 * from a socket).
 */
#ifndef ETIME
#define ETIME ETIMEDOUT
#endif

/*
 * Public utility routines.
 */
extern int getaddrlist(const char *, const char *, struct addrinfo **);
extern char *get_printserver(void);
extern struct addrinfo *get_printaddr(void);
extern ssize_t tread(int, void *, size_t, unsigned int);
extern ssize_t treadn(int, void *, size_t, unsigned int);
extern int connect_retry(int, int, int, const struct sockaddr *, socklen_t);
extern int initserver(int, const struct sockaddr *, socklen_t, int);

/**
 * Structure describing a print request.  This defines the protocol between the
 * print command and the printer spooling daemon to request a print job.
 */
struct printreq {
  /*
   * All integer sizes are defined with an explicit size to avoid misaligned
   * structure elements when a client has a different long integer size than
   * the server.
   */
  uint32_t size;           /* size in bytes */
  uint32_t flags;          /* request flag; defined below */
  char usernm[USERNM_MAX]; /* user's name */
  char jobnm[JOBNM_MAX];   /* print job's name */
};

/**
 * Request flags.  Treat file as plain text (instead of PostScript).  This is
 * defined as a bitmask of flags to allow extension of the protocol in the
 * future to support more characteristics.
 */
#define PR_TEXT 0x01

/**
 * The response from the spooling daemon to the print command.  This defines the
 * protocol between the printer spooling daemon and the print command.
 */
struct printresp {
  /*
   * All integer sizes are defined with an explicit size to avoid misaligned
   * structure elements when a client has a different long integer size than
   * the server.
   */
  uint32_t retcode;     /* 0 = success; !0 = error code */
  uint32_t jobid;       /* job ID */
  char msg[MSGLEN_MAX]; /* error message; if the request failed */
};

#endif /* _PRINT_H */
