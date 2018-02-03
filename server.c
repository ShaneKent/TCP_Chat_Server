#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "server.h"
#include "networks.h"
#include "helper.h"

#define DEBUG_FLAG 1
#define MAXBUF 32768

int main (int argc, char * argv[]) {
   
   int port_number = check_arguments(argc, argv);
   uint32_t server_socket = (uint32_t) tcpServerSetup(port_number);
   
   run(server_socket);

   close(server_socket);

   return EXIT_SUCCESS;
}

int check_arguments(int argc, char * argv[]) {

   int port_number = 0;

   if (argc > 2) {
      fprintf(stderr, "Usage: %s [port-number]\n", argv[0]);
      exit(EXIT_FAILURE);
   }

   if (argc == 2) {
      port_number = atoi(argv[1]);
   }

   return port_number;
}

void run (uint32_t server_socket) {
   
   struct server_info server;
   server.server_socket = server_socket;
   server.clients = NULL;

   struct client_ptr * temp_client;

   while (1) {
      
      set_and_select_file_descriptors(&server);

      if (FD_ISSET(server.server_socket, &server.rfds)) {
         new_client_connected(&server);
      }

      temp_client = server.clients;

      while (temp_client != NULL) {             // loop through clients.
         client_ready(&server, temp_client);
         temp_client = temp_client->next_client;
      }

   }
}

void set_and_select_file_descriptors (struct server_info * server) {
   
   uint32_t max_fd;
   struct timeval tv;
   struct client_ptr * current_client = server->clients;
   
   FD_ZERO(&server->rfds);
   FD_SET(server->server_socket, &server->rfds);
   max_fd = server->server_socket;

   while (current_client != NULL) {

      FD_SET(current_client->client_socket, &server->rfds);

      if (current_client->client_socket > max_fd)
         max_fd = current_client->client_socket;

      current_client = current_client->next_client;
   }
   
   tv.tv_sec = 0;
   tv.tv_usec = 500000;

   select(max_fd + 1, (fd_set *) &server->rfds, (fd_set *) 0, (fd_set *) 0, &tv);

}

void new_client_connected (struct server_info * server) {
   
   struct client_ptr * new_client;
   struct client_ptr * temp_client;

   server->number_clients += 1;     // increment num of clients stored

   new_client = (struct client_ptr *) malloc(sizeof(struct client_ptr));
   new_client->client_socket = tcpAccept(server->server_socket, 0);
   *new_client->client_handle = '\0'; // initialize handle as NULL
   new_client->next_client = NULL;
   
   if (server->clients == NULL) {
      server->clients = new_client; // no stored clients
   } else {
      temp_client = server->clients;       

      while (temp_client != NULL)   // get to the end of the client list
         temp_client = temp_client->next_client; 

      temp_client->next_client = new_client; // add new client

   }
   
   printf("New client connected on socket %d\n", new_client->client_socket);

   return;
}

void client_ready (struct server_info * server, struct client_ptr * client) {
   printf("Client ready: %d\n", client->client_socket);
}



void parse_client_message(uint32_t client_socket, uint8_t buf[], fd_set * rfds, uint32_t * fds, uint32_t * num_fds) {

   int len;
   uint8_t * ptr = buf;
   struct chat_header * hdr;

   if (FD_ISSET(client_socket, rfds)) {
      len = recv(client_socket, ptr, MAXBUF, MSG_DONTWAIT);
      
      hdr = (struct chat_header *) ptr;

      if (hdr->flag == 1) {
         printf("initialize\n");

      }
      
      int i;
      for (i = 0; i < len; i++)
         printf("%02x ", ptr[i]);
      printf("\n");
      fflush(stdout);

      printf("\tlength: %d, flags: %d\n", hdr->packet_len, hdr->flag);

      if (len == 0) {   // client closed forcefully.
         close(client_socket);
         remove_element(fds, *num_fds, client_socket);
         *num_fds -= 1;
         
         printf("Client %d closed - number sockets: %d\n", client_socket, *num_fds);
      }
   }

}






