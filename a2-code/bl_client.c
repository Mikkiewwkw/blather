#include "blather.h"

simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

client_t client_actual;
client_t *client = &client_actual;

pthread_t user_thread;          // thread managing user input
pthread_t server_thread;

// Worker thread to manage user input
void *user_worker(void *arg){
  int count = 1;
  while(!simpio->end_of_input){
    simpio_reset(simpio);
    iprintf(simpio, "");                                          // print prompt
    while(!simpio->line_ready && !simpio->end_of_input){          // read until line is complete
      simpio_get_char(simpio);
    }
    if(simpio->line_ready){
      mesg_t message;
      message.kind = BL_MESG;
      strcpy(message.name,client->name);
      strcpy(message.body,simpio->buf);

      char *name;
      sprintf(name,"%s.fifo",client->to_server_fname);
      int to_server_fd = open(name,O_RDWR);
      check_fail(to_server_fd < 0,1,"Can't open to_server_fd\n");

      int n_bytes = write(to_server_fd, &message,sizeof(mesg_t));
      check_fail(to_server_fd == -1,1,"Can't write to_server_fd\n");
      //iprintf(simpio, "%2d You entered: %s\n",count,simpio->buf);
      count++;
    }
  }

  mesg_t leaving_message;
  leaving_message.kind = BL_DEPARTED;
  pthread_cancel(server_thread); // kill the background thread
  return NULL;
}

// Worker thread to listen to the info from the server.
void *server_worker(void *arg){
  mesg_t message;
  int n_bytes = read(client->to_client_fd,&message,sizeof(message));
  check_fail(n_bytes < 0,1,"Server can read from client %s\n",client->name);
  while(1)
  {
    if(message.kind == BL_MESG)
    {
      iprintf(simpio,"[%s] : %s",message.name,message.body);
    }
    else if(message.kind == BL_JOINED)
    {
      iprintf(simpio,"-- %s JOINED --",message.name);
    }
    else if(message.kind == BL_DEPARTED)
    {
      iprintf(simpio,"-- %s DEPARTED --",message.name);
    }
    else if(message.kind == BL_SHUTDOWN)
    {
      iprintf(simpio,"!!! server is shutting down !!!");
      break;
    }
    else if(message.kind == BL_DISCONNECTED)
    {
      iprintf(simpio,"-- %s DEPARTED --",message.name);
    }
  }

  pthread_cancel(user_thread); // kill the background thread
  return NULL;
}


int main(int argc, char *argv[]){
  if (argc < 2)
  {
    printf("Not enough arguments\n");
    exit(0);
  }

  char prompt[MAXNAME];
  snprintf(prompt, MAXNAME, "%s>> ","fgnd"); // create a prompt string
  simpio_set_prompt(simpio, prompt);         // set the prompt
  simpio_reset(simpio);                      // initialize io
  simpio_noncanonical_terminal_mode();       // set the terminal into a compatible mode

  char server_name[MAXPATH];
  strcpy(server_name,argv[0]);
  char client_name[MAXPATH];
  strcpy(client_name,argv[1]);

  char *name;
  sprintf(name,"%s.fifo", server_name);
  int join_fd = open(name, O_RDWR);

  join_t join_request;
  strcpy(join_request.name,client_name);

  //create name for to_client_fd
  sprintf(join_request.to_client_fname, "%d", getpid());
  strcpy(client->to_client_fname,join_request.to_client_fname);
  char *to_client_name;
  sprintf(to_client_name,"%s.fifo",join_request.to_client_fname);
  int to_client_fifo = mkfifo(to_client_name, DEFAULT_PERMS);
  check_fail(to_client_fifo < 0, 1,"Couldn't create file %s", to_client_name);

  //create temporary file name for to_server_fd
  char *tempname;
  sprintf(join_request.to_server_fname, "%s", tmpnam(tempname));
  strcpy(client->to_server_fname,join_request.to_client_fname);
  char *to_server_name;
  sprintf(to_server_name,"%s.fifo",join_request.to_server_fname);
  int to_server_fifo = mkfifo(to_server_name, DEFAULT_PERMS);
  check_fail(to_server_fifo < 0,1, "Couldn't create file %s", to_server_name);


  pthread_create(&user_thread,   NULL, user_worker,   NULL);     // start user thread to read input
  pthread_create(&server_thread, NULL, server_worker, NULL);
  pthread_join(user_thread, NULL);
  pthread_join(server_thread, NULL);

  simpio_reset_terminal_mode();
  printf("\n");                 // newline just to make returning to the terminal prettier
  return 0;
}
