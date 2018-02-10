/*
 * Client functions used by open server.
 */
#include "opend.h"

#define NALLOC 10 /* # client structs to alloc/realloc for */

/**
 * Allocate more entries in the client[] array.
 */
static void client_alloc(void) {
  int i;

  if (client == NULL) {
    client = malloc(NALLOC * sizeof(Client));
  } else {
    client = realloc(client, (client_size + NALLOC) * sizeof(Client));
  }
  if (client == NULL) {
    err_sys("Can't alloc for new client array");
  }

  /* Initialise the new entries */
  for (i = client_size; i < client_size + NALLOC; i++) {
    client[i].fd = -1; /* fd of -1 means entry available */
  }
  client_size += NALLOC;
}

/**
 * Add a new client to the client array.  Called by loop() when connection
 * request from a new client arrives.
 * @param fd client file descriptor.
 * @param uid client process user ID.
 * @return index of added client in the client[] array.
 */
int client_add(int fd, uid_t uid) {
  int i;

  if (client == NULL) { /* first time called */
    client_alloc();
  }

again:
  for (i = 0; i < client_size; i++) {
    if (client[i].fd == -1) { /* find an available entry */
      client[i].fd = fd;
      client[i].uid = uid;
      return (i); /* return index in client[] array */
    }
  }

  /* Client array full, time to realloc for more */
  client_alloc();
  goto again; /* and search again (will work this time) */
}

/**
 * Remove a client from the client[] array.  Called by loop() when we're done
 * with a client.
 * @param fd client file descriptor to remove from client[] array.
 */
void client_del(int fd) {
  int i;

  for (i = 0; i < client_size; i++) {
    if (client[i].fd == fd) {
      client[i].fd = -1;
      return;
    }
  }
  log_quit("Can't find client entry for fd %d", fd);
}
