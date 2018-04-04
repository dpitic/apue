/*
 * Print server daemon.
 */
#include "apue.h"
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/uio.h>

#include "ipp.h"
#include "print.h"

/*
 * Macros used for the HTTP response from the printer.
 */
#define HTTP_INFO(x) ((x) >= 100 && (x) <= 199)
#define HTTP_SUCCESS(X) ((x) >= 200 && (x) <= 299)

/**
 * Structure used to describe a print job.
 */
struct job {
  struct job *next;    /* next job in list */
  struct job *prev;    /* previous job in list */
  int32_t jobid;       /* print job ID */
  struct printreq req; /* copy of print request */
};

/**
 * Structure used to describe a thread processing a client request.
 */
struct worker_thread {
  struct worker_thread *next; /* next in list */
  struct worker_thread *prev; /* previous in list */
  pthread_t tid;              /* thread ID */
  int sockfd;                 /* socket file descriptor */
};

/**
 * Logging flag: 0 = log messages to system log; 1 = log to stderr.
 */
int log_to_stderr = 0;

/*
 * Printer related variables.
 */
/** Stores the network address of the printer */
struct addrinfo *printer;
/** Host name of the printer */
char *printer_name;
/** Protect access to reread variable */
pthread_mutex_t configlock = PTHREAD_MUTEX_INITIALIZER;
/** Flag to indicate that daemon needs to reread the configuration file */
int reread;

/*
 * Thread related variables.
 */
/** Pointer to head of doubly linked list of threads that receive files from
 * clients. */
struct worker_thread *workers;
/** Protect access to workers list */
pthread_mutex_t workerlock = PTHREAD_MUTEX_INITIALIZER;
/** Signal mask used by the threads */
sigset_t mask;

/*
 * Job related variables.
 */
/** jobhead and jobtail are the start and end of the list of pending jobs */
struct job *jobhead, *jobtail;
/** File descriptor for the job file */
int jobfd;
/** ID of the next print job to be received by the print server. */
int32_t nextjob;
/** Mutex used to protect the linked list of jobs and the condiiton. */
pthread_mutex_t joblock = PTHREAD_MUTEX_INITIALIZER;
/** Condition variable for waiting jobs. */
pthread_cond_t jobwait = PTHREAD_COND_INITIALIZER;

/*
 * Function prototypes.
 */
void init_request(void);
void init_printer(void);
void update_jobno(void);
int32_t get_newjobno(void);
void add_job(struct printreq *, int32_t);
void replace_job(struct job *);
void remove_job(struct job *);
void build_qonstart(void);
void *client_thread(void *);
void *printer_thread(void *);
void *signal_thread(void *);
ssize_t readmore(int, char **, int, int *);
int printer_status(int, struct job *);
void add_worker(pthread_t, int);
void kill_workers(void);
void client_cleanup(void *);

/*
 * Main print server thread.  Accepts connect requests from clients and spawns
 * additional threads to service requests.
 */
int main(int argc, char *argv[]) {
  pthread_t tid;
  struct addrinfo *ailist, *aip;
  int sockfd, err, i, n, maxfd;
  char *host;
  fd_set rendezvous, rset;
  struct sigaction sa;
  struct passwd *pwdp;

  if (argc != 1) {
    err_quit("Usage: %s", argv[0]);
  }
  /*
   * Become daemon; can no longer print error messages to stderr, can only log
   * errors from now on.
   */
  daemonize("printd");

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  /*
   * Will be writing to socket file descriptors, so don't want a write error to
   * trigger SIGPIPE, because the default action is to kill the process.
   */
  if (sigaction(SIGPIPE, &sa, NULL) < 0) {
    log_sys("sigaction() failed");
  }
  /*
   * Set the signal mask of the thread to include SIGHUP and SIGTERM.  All
   * threads created will inherit this signal mask.  The signals are used to
   * control the daemon as follows:
   *   SIGHUP: tell daemon to reread its configuration file.
   *   SIGTERM: tell daemon to clean up and exit gracefully.
   */
  sigemptyset(&mask);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGTERM);
  if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0) {
    log_sys("pthread_sigmask() failed");
  }

  /* Get the maximum size of a host name */
  n = sysconf(_SC_HOST_NAME_MAX);
  if (n < 0) {
    n = HOST_NAME_MAX; /* best guess */
  }
  /* Allocate memory to hold the host name */
  if ((host = malloc(n)) == NULL) {
    log_sys("malloc() error");
  }
  if (gethostname(host, n) < 0) {
    log_sys("gethostname() error");
  }

  /*
   * Try to find the network address of the daemon providing the printer
   * spooling service.
   */
  if ((err = getaddrlist(host, "print", &ailist)) != 0) {
    log_quit("getaddrinfo() error: %s", gai_strerror(err));
    exit(1);
  }

  /* Clear the fd set used to wait for client connect requests */
  FD_ZERO(&rendezvous);
  maxfd = -1; /* ensure first file descriptor allocated is > maxfd */
  /*
   * Call initserver() on each network address that the print service needs to
   * be provide on, to initialise a socket.
   */
  for (aip = ailist; aip != NULL; aip = aip->ai_next) {
    if ((sockfd = initserver(SOCK_STREAM, aip->ai_addr, aip->ai_addrlen,
                             QLEN)) >= 0) {
      /*
       * Add the file descriptor to the fd set used to wait for client connect
       * requests.
       */
      FD_SET(sockfd, &rendezvous);
      if (sockfd > maxfd) {
        maxfd = sockfd;
      }
    }
  }
  if (maxfd == -1) {
    /* Can't enable printer spooling service; log message and quit */
    log_quit("service not enabled");
  }

  /*
   * Daemon needs root privileges to bind a socket to a reserved port number.
   * Now this is done, lower its privileges by changing its user and group ID
   * to the one associated with the LPNAME account.
   */
  pwdp = getpwnam(LPNAME);
  if (pwdp == NULL) {
    log_sys("Can't find user %s", LPNAME);
  }
  if (pwdp->pw_uid == 0) {
    log_quit("User %s is privileged", LPNAME);
  }
  /* Try to change the real and effective IDs to LPNAME account */
  if (setgid(pwdp->pw_gid) < 0 || setuid(pwdp->pw_uid) < 0) {
    log_sys("Can't change IDs to user %s", LPNAME);
  }

  init_request(); /* initialise job requests & ensure only 1 daemon running */
  init_printer(); /* initialise printer information */

  /* Create thread to communicate with the printer */
  err = pthread_create(&tid, NULL, printer_thread, NULL);
  /* Create thread to handle signals */
  if (err == 0) {
    err = pthread_create(&tid, NULL, signal_thread, NULL);
  }
  if (err != 0) {
    log_exit(err, "Can't create thread");
  }
  /*
   * Search the printer spool directory for any pending print jobs.  For each
   * job found on disk, a strucutre is created to let the printer thread know
   * that it should send the file to the printer.
   */
  build_qonstart();

  /* Finished setting up the print spooling daemon */
  log_msg("Daemon initialised");

  /* main thread infinite loop */
  for (;;) {
    /*
     * select() modifies fd set passed to it to include only those fds that
     * satisfy the event, so make a copy of rendezvous set.
     */
    rset = rendezvous;
    /* Wait for one of the file descriptors to become readable */
    if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) {
      log_sys("select() failed");
    }
    /* Check rset for a readable file descriptor */
    for (i = 0; i <= maxfd; i++) {
      if (FD_ISSET(i, &rset)) {
        /*
         * Accept the connection and handle the request.
         */
        if ((sockfd = accept(i, NULL, NULL)) < 0) {
          log_ret("accept() failed");
        }
        /* Create thread to handle client connection */
        pthread_create(&tid, NULL, client_thread, (void *)((long)sockfd));
      }
    }
  }
  /* main thread should never reach this exit statement */
  exit(1);
} /* main() */

/**
 * @brief Initialise the job ID file.
 *
 * Use a record lock to prevent more than one printer daemon from running at a
 * time.
 */
void init_request(void) {
  int n;
  char name[FILENMSZ];

  sprintf(name, "%s/%s", SPOOLDIR, JOBFILE);
  jobfd = open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  /*
   * Obtain write lock on job file to prevent other instances of the print spool
   * daemon running.
   */
  if (write_lock(jobfd, 0, SEEK_SET, 0) < 0) {
    log_quit("Daemon already running");
  }

  /*
   * Reuse the name buffer for the job counter.  Job file contains an ASCII
   * integer string representing the next job number.  If the file was newly
   * created, it will be empty.
   */
  if ((n = read(jobfd, name, FILENMSZ)) < 0) {
    log_sys("Can't read job file");
  }
  if (n == 0) { /* New file is empty; set nextjob number to 1 */
    nextjob = 1;
  } else { /* Existing file; get job number (from reused buffer) */
    nextjob = atol(name);
  }
} /* init_request*() */

/**
 * @brief Initialise printer information from configuration file.
 *
 * This function is used to set the printer name and address.
 */
void init_printer(void) {
  printer = get_printaddr();
  if (printer == NULL) {
    exit(1); /* message already logged */
  }
  printer_name = printer->ai_canonname;
  if (printer_name == NULL) {
    /* Printer name not defined; use some default name */
    printer_name = "printer";
  }
  log_msg("printer is %s", printer_name);
} /* init_printer() */

/**
 * @brief Update the job ID file with the next job number.
 * @details This function is used to write the next job number to the job file.
 * It does not handle wrap-around of job number.
 */
void update_jobno(void) {
  char buf[32];

  if (lseek(jobfd, 0, SEEK_SET) == -1) {
    log_sys("Can't seek in job file");
  }
  sprintf(buf, "%d", nextjob);
  if (write(jobfd, buf, strlen(buf)) < 0) {
    log_sys("Can't update job file");
  } /* update_jobno() */
}

/**
 * @brief Get the next job number.
 *
 * @return next job number.
 */
int32_t get_newjobno(void) {
  int32_t jobid;

  pthread_mutex_lock(&joblock);
  jobid = nextjob++;
  /* Handle case where jobid wraps around; restart at 1 */
  if (nextjob <= 0) {
    nextjob = 1;
  }
  pthread_mutex_unlock(&joblock);
  return (jobid);
} /* get_newjobno() */

/**
 * @brief Add a new job to the list of pending jobs.
 *
 * This function is used to add a new job to the list of pending jobs and then
 * signal the printer thread that a job is pending.
 *
 * @param reqp pointer to printreq structure from client.
 * @param jobid print job number.
 */
void add_job(struct printreq *reqp, int32_t jobid) {
  struct job *jp;

  if ((jp = malloc(sizeof(struct job))) == NULL) {
    log_sys("malloc() failed");
  }
  /* Copy request struct from the client into the job structure */
  memcpy(&jp->req, reqp, sizeof(struct printreq));
  jp->jobid = jobid;
  jp->next = NULL;
  pthread_mutex_lock(&joblock);
  jp->prev = jobtail;    /* set new struct prev ptr to last job on list */
  if (jobtail == NULL) { /* List is empty */
    jobhead = jp;        /* job head ptr; this job is at the head of the list */
  } else {               /* List not empty */
    jobtail->next = jp;  /* next ptr of last entry points to this job */
  }
  jobtail = jp; /* job tail ptr; this job is at the end of the list */
  pthread_mutex_unlock(&joblock);
  pthread_cond_signal(&jobwait); /* signal print thread another job avail. */
} /* add_job() */

/**
 * @brief Replace a job back on the head of the list.
 * @details This function is used to insert a job at the head of the pending
 * job list.
 * @param jp pointer to job that is to be inserted on the head of the list.
 */
void replace_job(struct job *jp) {
  pthread_mutex_lock(&joblock);
  jp->prev = NULL;       /* this job is at the head of the job list */
  jp->next = jobhead;    /* insert this job at the head of the list */
  if (jobhead == NULL) { /* empty list */
    jobtail = jp;        /* this is the only job in the list */
  } else {               /* job list not empty */
    jobhead->prev = jp;  /* insert this job at the head of the list */
  }
  jobhead = jp; /* insert this job at the head of the job list */
  pthread_mutex_unlock(&joblock);
} /* replace_job() */

/**
 * @brief Remove a job from the list of pending jobs.
 * @details This function removes a job from the list of pending jobs given a
 * pointer to the job to be removed.  The caller must hold the job lock mutex.
 * @param target pointer to job that should be removed from job list.
 */
void remove_job(struct job *target) {
  /* Set this target job's previous pointer */
  if (target->next != NULL) { /* target is not the last job in the job list */
    target->next->prev = target->prev;
  } else { /* target job is the last job in the job list */
    jobtail = target->prev;
  }
  /* Set this target job's next pointer */
  if (target->prev != NULL) { /* target is not the first job in the job list */
    target->prev->next = target->next;
  } else { /* target job is the first job in the job list */
    jobhead = target->next;
  }
} /* remove_job() */

/**
 * @brief Check the spool directory for pending jobs on start-up.
 * @details When the print spooler daemon starts, it uses this function to
 * build an in-memory list of print jobs from the disk files stored in the
 * print spooler printer request directory.  If the directory can't be opened,
 * no print jobs are pending, so the function returns.
 */
void build_qonstart(void) {
  int fd, err, nr;
  int32_t jobid;
  DIR *dirp;
  struct dirent *entp;
  struct printreq req;
  char dname[FILENMSZ], fname[FILENMSZ];

  sprintf(dname, "%s/%s", SPOOLDIR, REQDIR);
  if ((dirp = opendir(dname)) == NULL) {
    /* No print jobs are pending */
    return;
  }
  /* Read each entry in the directory, one at a time; skip . and .. */
  while ((entp = readdir(dirp)) != NULL) {
    /*
     * Skip "." and ".."
     */
    if (strcmp(entp->d_name, ".") == 0 || strcmp(entp->d_name, "..") == 0) {
      continue;
    }
    /*
     * Read the request structure.
     */
    /* Build full pathname of the print request file and open for reading */
    sprintf(fname, "%s/%s/%s", SPOOLDIR, REQDIR, entp->d_name);
    if ((fd = open(fname, O_RDONLY)) < 0) {
      /* Opening file failed; just skip the file */
      continue;
    }
    /* Read the print request structure from the file */
    nr = read(fd, &req, sizeof(struct printreq));
    /* Check to ensure whole strucutre was read */
    if (nr != sizeof(struct printreq)) {
      if (nr < 0) {
        err = errno;
      } else {
        err = EIO;
      }
      close(fd);
      log_msg("build_qonstart(): can't read %s: %s", fname, strerror(err));
      unlink(fname);
      /* Build full pathname of corresponding data file */
      sprintf(fname, "%s/%s/%s", SPOOLDIR, DATADIR, entp->d_name);
      unlink(fname);
      continue;
    }
    /* Able to read the complete print request strucutre */
    jobid = atol(entp->d_name); /* get print job ID */
    log_msg("Adding job %d to queue", jobid);
    add_job(&req, jobid); /* Add request to list of pending print jobs */
  }
  /*
   * Finished reading the directory; readdir() will return NULL; close directory
   * and return.
   */
  closedir(dirp);
} /* build_qonstart() */

/**
 * @brief Accept a print job from a client.
 * @details Client thread is spawned from the main thread when a connect request
 * is accepted.  Its job is to receive the file to be printed from the client
 * print command.  A separate thread is created for each client print request.
 *
 * @param arg socket file descriptor.
 */
void *client_thread(void *arg) {
  int n, fd, sockfd, nr, nw, first;
  int32_t jobid;
  pthread_t tid;
  struct printreq req;
  struct printresp res;
  char name[FILENMSZ];
  char buf[IOBUFSZ];

  tid = pthread_self();
  /* Install thread cleanup handler */
  pthread_cleanup_push(client_cleanup, (void *)((long)tid));
  sockfd = (long)arg;
  /* Create worker thread structure & add it to list of active client threads */
  add_worker(tid, sockfd);

  /*
   * Read the request header.
   */
  if ((n = treadn(sockfd, &req, sizeof(struct printreq), 10)) !=
      sizeof(struct printreq)) {
    res.jobid = 0;
    if (n < 0) {
      res.retcode = htonl(errno);
    } else {
      res.retcode = htonl(EIO);
    }
    strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
    writen(sockfd, &res, sizeof(struct printresp));
    pthread_exit((void *)1);
  }
  req.size = ntohl(req.size);
  req.flags = ntohl(req.flags);

  /*
   * Create the data file.
   */
  jobid = get_newjobno();
  sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jobid);
  fd = creat(name, FILEPERM);
  if (fd < 0) {
    res.jobid = 0;
    res.retcode = htonl(errno);
    log_msg("client_thread(): can't create %s: %s", name,
            strerror(res.retcode));
    strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
    writen(sockfd, &res, sizeof(struct printresp));
    pthread_exit((void *)1);
  }

  /*
   * Read the file and store it in the spool directory.  Try to figure out if
   * the file is a PostScript file or a plain text file.
   */
  first = 1;
  while ((nr = tread(sockfd, buf, IOBUFSZ, 20)) > 0) {
    if (first) {
      first = 0;
      if (strncmp(buf, "%!PS", 4) != 0) {
        /* The file doesn't begin with the pattern %!PS; assume text file */
        req.flags |= PR_TEXT;
      }
    }
    nw = write(fd, buf, nr);
    if (nw != nr) {
      res.jobid = 0;
      if (nw < 0) {
        res.retcode = htonl(errno);
      } else {
        res.retcode = htonl(EIO);
      }
      log_msg("client_thread(): can't write %s: %s", name,
              strerror(res.retcode));
      close(fd);
      strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
      writen(sockfd, &res, sizeof(struct printresp));
      unlink(name);
      pthread_exit((void *)1);
    }
  }
  close(fd);

  /*
   * Create the control file.  Then write the print request information to the
   * control file.
   */
  sprintf(name, "%s/%s/%d", SPOOLDIR, REQDIR, jobid);
  fd = creat(name, FILEPERM);
  if (fd < 0) {
    /*
     * Print request control file creation failed; remove data file & terminate
     * thread.
     */
    res.jobid = 0;
    res.retcode = htonl(errno);
    log_msg("client_thread(): can't create %s: %s", name,
            strerror(res.retcode));
    strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
    writen(sockfd, &res, sizeof(struct printresp));
    sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jobid);
    unlink(name);
    pthread_exit((void *)1);
  }

  /* Write print request structure to the control file. */
  nw = write(fd, &req, sizeof(struct printreq));
  if (nw != sizeof(struct printreq)) {
    res.jobid = 0;
    if (nw < 0) {
      res.retcode = htonl(errno);
    } else {
      res.retcode = htonl(EIO);
    }
    log_msg("client_thread(): can't write %s: %s", name, strerror(res.retcode));
    close(fd);
    strncpy(res.msg, strerror(res.retcode), MSGLEN_MAX);
    writen(sockfd, &res, sizeof(struct printresp));
    unlink(name);
    sprintf(name, "%s/%s/%d", SPOOLDIR, DATADIR, jobid);
    unlink(name);
    pthread_exit((void *)1);
  }
  /*
   * Close file descriptor for control file.  File descriptors are not
   * automatically closed when a thread ends if other threads exist in the
   * process.
   */
  close(fd);

  /*
   * Send response back to client.
   */
  res.retcode = 0; /* successful status */
  res.jobid = htonl(jobid);
  sprintf(res.msg, "Request ID %d", jobid);
  writen(sockfd, &res, sizeof(struct printresp));

  /*
   * Notify the printer thread, clean up, and exit.
   */
  log_msg("Adding job %d to queue", jobid);
  add_job(&req, jobid); /* add job to list of pending print jobs */
  pthread_cleanup_pop(1);
  return ((void *)0);
} /* client_thread() */
