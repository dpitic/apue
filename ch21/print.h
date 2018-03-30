#ifndef _PRINT_H
#define _PRINT_H

/*
 * Print server header file.
 */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

/**
 * Printer configuration file contains the host names of the printer spooling
 * daemon and the network attached printer.
 */
#define CONFIG_FILE		"./etc/printer.conf"
/**
 * Directories used by the implementation must be created by an administrator
 * and be owned by the same user account under which the printer spooling daemon
 * runs.  The daemon won't try to create these directories if they don't exits,
 * because the daemon would need root privileges to create directories in
 * /var/spool.  The daemon is designed to do as little as possible while running
 * as root to minimise the chance of creating a security hole.
 */
#define SPOOLDIR			"./var/spool/printer"
/**
 * Directory that holds file containing next job number; appended to SPOOLDIR.
 */
#define JOBFILE				"jobno"
/**
 * Directory that holds copies of files to be printed; appended to SPOOLDIR.
 */
#define DATADIR				"data"
/**
 * Directory that holds control information for each request; appended to
 * SPOOLDIR.
 */
#define REQDIR				"reqs"

#if defined(BSD)
#define LPNAME				"daemon"
#elif defined(MACOS)
#define LPNAME				"_lp"
#else
#define LPNAME				"lp"
#endif

#endif /* _PRINT_H */
