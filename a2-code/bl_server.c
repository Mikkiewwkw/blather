#include "blather.h"

int sig_flag = 0; //indicate if a SIGINT or SIGTERM needs to be handled

void handle_SIGINT(int sig_num)
{
  sig_flag = 1;
}

void handle_SIGTERM(int sig_num)
{
  sig_flag = 1;
}

int main(int argc, char **argv)
{
  struct sigaction my_sa = {};
  my_sa.sa_handler = handle_SIGINT;
  sigaction(SIGINT, &my_sa, NULL);

  my_sa.sa_handler = handle_SIGTERM;
  sigaction(SIGTERM, &my_sa, NULL);

  server_t *server;
  server_start(server,argv[0],DEFAULT_PERMS);
  while(1)
  {
    if(sig_flag)
    {
      break;
    }

    server_check_sources(server);
    if (server_join_ready(server))
    {
      server_handle_join(server);
    }

    int number_clients = server->n_clients;
    for(int i=0;i<number_clients;i++)
    {
      if(sig_flag)
      {
        break;
      }

      if(server_client_ready(server,i))
      {
        server_handle_client(server,i);
      }
    }
    server_shutdown(server);
    return 0;
  }
}
