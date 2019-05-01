#include "blather.h"

client_t *server_get_client(server_t *server, int idx)
{
  check_fail(idx > (server->n_clients),0,"The index is beyond number of clients\n");

  client_t client = server->client[idx];
  return client;
}
// Gets a pointer to the client_t struct at the given index. If the
// index is beyond n_clients, the behavior of the function is
// unspecified and may cause a program crash.

void server_start(server_t *server, char *server_name, int perms)
{
  strcpy(server->server_name,server_name);
  char* name;
  sprintf(name,"%s.fifo", server_name);
  remove(name);                                               // remove old FIFO that has the server_name, ensures starting in a good state
  int myfd = mkfifo(name, perms);                          // create join FIFO for clients
  check_fail(myfd < 0, "Couldn't create file %s", name);

  int join_fd = open(name, O_RDWR);
  check_fail(join_fd < 0, "Couldn't open file %s", name);
  server->join_ffd_set my_set;void FD_ZERO(fd_set *set);// clear entire setvoid FD_SET(int fd, fd_set *set);   // fd now in setvoid FD_CLR(int fd, fd_set *set);   // fd now not in setint  FD_ISSET(int fd, fd_set *set)d = join_fd;
  server->join_ready = 0;
  server->n_clients = 0;
}
// Initializes and starts the server with the given name. A join fifo
// called "server_name.fifo" should be created. Removes any existing
// file of that name prior to creation. Opens the FIFO and stores its
// file descriptor in join_fd._
//
// ADVANCED: create the log file "server_name.log" and write the
// initial empty who_t contents to its beginning. Ensure that the
// log_fd is position for appending to the end of the file. Create the
// POSIX semaphore "/server_name.sem" and initialize it to 1 to
// control access to the who_t portion of the log.

void server_shutdown(server_t *server)
{
  close(server->join_fd);
  remove(server->server_name);

  mesg_t *mesg = NULL;
  mesg->kind = BL_SHUTDOWN;
  server_broadcast(server, mesg);

  int count = server->n_clients;
  for(int i = 0; i < count; i++)
  {
    server_remove_client(server,i);
  }
  printf("!!! server is shutting down !!!\n");
}
// Shut down the server. Close the join FIFO and unlink (remove) it so
// that no further clients can join. Send a BL_SHUTDOWN message to all
// clients and proceed to remove all clients in any order.
//
// ADVANCED: Close the log file. Close the log semaphore and unlink
// it.

int server_add_client(server_t *server, join_t *join)
{
  if (server->n_clients == MAXCLIENTS)
  {
    printf("No space for clients\n");
    return -1;
  }

  client_t new_client;
  strcpy(new_client.name,join->name);
  new_client.data_ready = 0;

  int to_client_fifo = mkfifo(join->to_client_fname, DEFAULT_PERMS);
  char *to_client_name;
  sprintf(to_client_name, "%s.fifo",join->to_client_fname)
  check_fail(to_client_fifo == -1, 1, "Couldn't create file %s",to_client_name);

  int to_client_fd = open(to_client_name,O_RDWR);
  check_fail(to_client_fd < 0, 1, "Couldn't open file %s", to_client_name);
  new_client.to_client_fd = to_client_fd;
  strcpy(new_client.to_client_fname,join->to_client_fname);


  int to_server_fifo = mkfifo(join->to_server_fname, DEFAULT_PERMS);
  char *to_server_name;
  sprintf(to_server_name, "%s.fifo",join->to_server_fname)
  check_fail(to_server_fifo == -1, 1, "Couldn't create file %s",to_server_name);

  int to_server_fd = open(to_server_name,O_RDWR);
  check_fail(to_server_fd < 0, 1, "Couldn't open file %s", to_server_name);
  new_client.to_server_fd = to_server_fd;
  strcpy(new_client.to_server_fname,join->to_client_fname);

  if(!server->join_ready)
  {
    printf("Join is not available\n");
    return -1;
  }

  int index = server->n_clients;
  server->client[index] = new_client;
  (server->n_clients)++;

  return 0;

}
// Adds a client to the server according to the parameter join which
// should have fileds such as name filed in.  The client data is
// copied into the client[] array and file descriptors are opened for
// its to-server and to-client FIFOs. Initializes the data_ready field
// for the client to 0. Returns 0 on success and non-zero if the
// server as no space for clients (n_clients == MAXCLIENTS).

int server_remove_client(server_t *server, int idx)
{
  client_t *rm_client = server->client[idx];
  close(rm_client->to_client_fd);
  close(rm_client->to_server_fd);
  remove(rm_client.name);

  (server->n_clients)--;
  int count = server->n_clients;
  int new_index = idx;
  int old_index = idx + 1;
  client_t *client = server->client;
  while(old_index < count)
  {
    client[new_index] = client[old_index];
    new_index = old_index;
    old_index++;
  }
  return 0;
}
// Remove the given client likely due to its having departed or
// disconnected. Close fifos associated with the client and remove
// them.  Shift the remaining clients to lower indices of the client[]
// preserving their order in the array; decreases n_clients.

int server_broadcast(server_t *server, mesg_t *mesg)
{
  int count = server->n_clients;
  for(int i = 0;i < count;i++)
  {
    client_t *client = server_get_client(server,i);
    int n_bytes = write(client->to_client_fd, mesg, sizeof(mesg_t));
    check_fail(n_bytes == -1,1,"Cant't send message to %s", client->name);
  }
  return 0;
}
// Send the given message to all clients connected to the server by
// writing it to the file descriptors associated with them.
//
// ADVANCED: Log the broadcast message unless it is a PING which
// should not be written to the log.

void server_check_sources(server_t *server)
{
  int maxfd = 0;
  fd_set my_set;
  FD_ZERO(my_set);
  FD_SET(server->join_fd, my_set);
  maxfd = server->join_fd;

  int count = server->n_clients;
  for(int i = 0;i<count;i++)
  {
    client_t *client = server_get_client(server,i);
    FD_SET(client->to_server_fd, my_set);
    maxfd = (maxfd < (client->to_server_fd)) ? (client->to_server_fd) : maxfd;
  }

  select(maxfd+1,&my_set,NULL,NULL,NULL);

  if(FD_ISSET(server->join_fd, &my_set))
  {
    server->join_ready = 1;
  }

  for(int j = 0;j<count;j++)
  {
    client = server_get_client(server,i);
    if(FD_ISSET(client->to_server_fd, &my_set))
    {
      client->data_ready = 1;
    }
  }
}
// Checks all sources of data for the server to determine if any are
// ready for reading. Sets the servers join_ready flag and the
// data_ready flags of each of client if data is ready for them.
// Makes use of the select() system call to efficiently determine
// which sources are ready.

int server_join_ready(server_t *server)
{
  int flag = server->join_ready;
  return flag;
}
// Return the join_ready flag from the server which indicates whether
// a call to server_handle_join() is safe.

int server_handle_join(server_t *server)
{
  checkfail(server_join_ready == 0,0,"Server can't be joined\n");

  join_t request;
  int n_bytes = read(server->join_fd,&request,sizeof(join_t));
  check_fail(n_bytes == -1,1,"Fail to read the join request\n");
  server->join_ready = 0;
  return 0;
}
// Call this function only if server_join_ready() returns true. Read a
// join request and add the new client to the server. After finishing,
// set the servers join_ready flag to 0.

int server_client_ready(server_t *server, int idx)
{
  client_t *client = server_get_client(server,idx);
  return (client->data_ready);
}
// Return the data_ready field of the given client which indicates
// whether the client has data ready to be read from it.

int server_handle_client(server_t *server, int idx)
{
  checkfail(server_client_ready == 0,0,"Nothing to read from client\n");

  client_t *client = server_get_client(server,idx);
  mesg_t message;
  int n_bytes = read(client->to_server_fd,&message,sizeof(mesg_t));
  check_fail(n_bytes<0,1,"Fail to read from client %s\n", client->name);

  if((message.kind == BL_DEPARTED) || (message.kind == BL_MESG) || (message.kind == BL_JOINED) || (message.kind == BL_DISCONNECTED))
  {
    server_broadcast(server,message);
  }
  else
  {
    printf("Message not specified\n", );
  }
  client->data_ready = 0;
  client->last_contact_time = server->time_sec;
  return 0;
}
// Process a message from the specified client. This function should
// only be called if server_client_ready() returns true. Read a
// message from to_server_fd and analyze the message kind. Departure
// and Message types should be broadcast to all other clients.  Ping
// responses should only change the last_contact_time below. Behavior
// for other message types is not specified. Clear the client's
// data_ready flag so it has value 0.
//
// ADVANCED: Update the last_contact_time of the client to the current
// server time_sec.

void server_tick(server_t *server);
{
  (server->time_sec)++;
}
// ADVANCED: Increment the time for the server

void server_ping_clients(server_t *server);
// ADVANCED: Ping all clients in the server by broadcasting a ping.

void server_remove_disconnected(server_t *server, int disconnect_secs);
// ADVANCED: Check all clients to see if they have contacted the
// server recently. Any client with a last_contact_time field equal to
// or greater than the parameter disconnect_secs should be
// removed. Broadcast that the client was disconnected to remaining
// clients.  Process clients from lowest to highest and take care of
// loop indexing as clients may be removed during the loop
// necessitating index adjustments.

void server_write_who(server_t *server);
// ADVANCED: Write the current set of clients logged into the server
// to the BEGINNING the log_fd. Ensure that the write is protected by
// locking the semaphore associated with the log file. Since it may
// take some time to complete this operation (acquire semaphore then
// write) it should likely be done in its own thread to preven the
// main server operations from stalling.  For threaded I/O, consider
// using the pwrite() function to write to a specific location in an
// open file descriptor which will not alter the position of log_fd so
// that appends continue to write to the end of the file.

void server_log_message(server_t *server, mesg_t *mesg);
// ADVANCED: Write the given message to the end of log file associated
// with the server.
