#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "server.h"
#include "networks.h"
#include "helper.h"

#define MAXBUF 3527

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
         if (FD_ISSET(temp_client->client_socket, &server.rfds))
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

      while (temp_client->next_client != NULL)   // get to the end of the client list
         temp_client = temp_client->next_client; 

      temp_client->next_client = new_client; // add new client

   }
   
   printf("New client connected on socket %d\n", new_client->client_socket);

   return;
}

void delete_client(struct server_info * server, struct client_ptr * client) {
   /*
    * There are 2 cases we need to check for:
    *  (1) Trying to remove the first node.
    *  (2) Trying to remove some other node.
    */

   struct client_ptr * c = server->clients;
   struct client_ptr * temp;

   // First Case
   if (c->client_socket == client->client_socket) { 
      server->clients = c->next_client;
      server->number_clients -= 1;

      close(client->client_socket);
      free(c);
      
      return;
   }

   // Second Case
   while (c->next_client != NULL) {
      
      if (c->next_client->client_socket == client->client_socket) {
         temp = c->next_client;
         
         c->next_client = c->next_client->next_client;
         
         server->number_clients -= 1;

         close(client->client_socket);
         free(temp);

         return;
      }

      c = c->next_client;

   }

}

void client_ready (struct server_info * server, struct client_ptr * client) {
   
   uint8_t buf[MAXBUF];
   struct chat_header * c_hdr;
   uint32_t len = recv(client->client_socket, buf, MAXBUF, 0);
   
   if (len < 0) {
      perror("recv call");
      exit(EXIT_FAILURE);
   } else if (len == 0) { // Time to close a client.
      delete_client(server, client);
      return;
   }

   printf("Received: ");
   print_buffer(buf, len);

   c_hdr = (struct chat_header *) buf;
   
   if (c_hdr->flag == 1) {

      flag_one(server, client, buf);

   }

   //print_all_clients(server);

}

void flag_one (struct server_info * server, struct client_ptr * client, uint8_t buf[]) {
   
   struct client_ptr * temp;
   struct initialize_packet * init_packet = (struct initialize_packet *) buf;
   uint8_t sender_handle[100] = {0};   // max string length.

   memcpy( &sender_handle, &init_packet->sender_handle_len + 1, init_packet->sender_handle_len );
   //printf("%s\n", sender_handle);

   // Let's check to see if the handle has been used.
   temp = server->clients;
   while (temp != NULL) {
      //printf("String 1: %s\nString 2: %s\n", sender_handle, temp->client_handle);
      if (strcmp( (char *) &sender_handle, (char *) &temp->client_handle) == 0) {
         bad_handle(server, client);
         return;
      }
      temp = temp->next_client;
   }
   
   // I wanted to put this inside 'good_handle', but I was having trouble doing that..
   strcpy( (char *) client->client_handle, (char *) sender_handle);

   good_handle(server, client);
}

void bad_handle (struct server_info * server, struct client_ptr * client) {
   
   struct chat_header c_hdr[3];
   
   c_hdr->packet_len = htons(3);
   c_hdr->flag = 3;

   wrapped_send(client->client_socket, c_hdr, 3, 0);
   
   delete_client(server, client);
}

void good_handle (struct server_info * server, struct client_ptr * client) {
   
   struct chat_header c_hdr[3];

   c_hdr->packet_len = htons(3);
   c_hdr->flag = 2;

   wrapped_send(client->client_socket, c_hdr, 3, 0);
}





void print_all_clients (struct server_info * server) {

   struct client_ptr * c = server->clients;

   printf("\n-----\nServer Info:\n\tServer Socket: %d\tNumber Clients: %d\n", server->server_socket, server->number_clients);

   while (c != NULL) {
      printf("\tClient Info:\n\t\tClient Socket: %d\n\t\tClient Handle: %s\n", c->client_socket, c->client_handle);
      c = c->next_client;
   }
   
   printf("-----\n\n");

}

