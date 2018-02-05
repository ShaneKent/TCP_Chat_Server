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

#define MAXBUF 6500

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
   
   //printf("New client connected on socket %d\n", new_client->client_socket);

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
   uint8_t * ptr;
   struct chat_header * c_hdr;
   uint32_t len = recv(client->client_socket, buf, MAXBUF, 0);
   uint32_t temp;

   check_recv_len(server, client, len);
   
   ptr = (uint8_t *) buf;

   while (len > 0) {
      
      c_hdr = (struct chat_header *) ptr;

      // This is the case where the length of the next packet is greater than the data currently received.
      uint16_t l = ntohs( (ptr[0] << 8) + ptr[1] );
      if (len < l) { //c_hdr->packet_len)) {
         temp = len;
         len += recv(client->client_socket, ptr + temp, MAXBUF, 0);
         check_recv_len(server, client, len);
      }
      
      // May want to make this a switch statement. Upon some research...
      // https://stackoverflow.com/questions/6805026/is-switch-faster-than-if
      if (c_hdr->flag == 1) {
         flag_one(server, client, ptr);
      } else if (c_hdr->flag == 5) {
         flag_five(server, client, ptr);
      } else if (c_hdr->flag == 8) {
         flag_eight(server, client, ptr);
      } else if (c_hdr->flag == 10) {
         flag_ten(server, client, ptr);
      }
      
      ptr += l;//ntohs(c_hdr->packet_len);
      len -= l;//ntohs(c_hdr->packet_len);
   }

}

void check_recv_len(struct server_info * server, struct client_ptr * client, uint32_t len) {
   
   if (len < 0) {
      perror("recv call");
      exit(EXIT_FAILURE);
   } else if (len == 0) { // Time to close a client.
      delete_client(server, client);
      return;
   }

}

void flag_one (struct server_info * server, struct client_ptr * client, uint8_t buf[]) {
   
   struct initialize_packet * init_packet = (struct initialize_packet *) buf;
   uint8_t sender_handle[100] = {0};   // max string length.

   memcpy( &sender_handle, &init_packet->sender_handle_len + 1, init_packet->sender_handle_len );

   if (handle_exists(server, sender_handle) == 0) {
      bad_handle(server, client);
      return;
   }
   
   // I wanted to put this inside 'good_handle', but I was having trouble doing that..
   strcpy( (char *) client->client_handle, (char *) sender_handle);

   good_handle(server, client);
}

void bad_handle (struct server_info * server, struct client_ptr * client) {
   
   uint8_t buf[3];

   buf[0] = htons(3) >> 8;
   buf[1] = htons(3);
   buf[2] = 3;

   wrapped_send(client->client_socket, buf, 3, 0);
   
   delete_client(server, client);
}

void good_handle (struct server_info * server, struct client_ptr * client) {
   
   uint8_t buf[3];
   
   buf[0] = htons(3) >> 8;
   buf[1] = htons(3);
   buf[2] = 2;

   wrapped_send(client->client_socket, buf, 3, 0);
}

void flag_five(struct server_info * server, struct client_ptr * client, uint8_t buf[]) {
  
   //struct chat_header * c_hdr = (struct chat_header *) buf;
   uint16_t curr_pos = 3;
   
   uint8_t sender_handle[100] = {0};   // max string length.
   uint8_t sender_len = buf[curr_pos]; // sender len after c_hdr;
   uint8_t number_dest;

   uint8_t dest_handle[100] = {0};
   uint8_t dest_len;

   curr_pos += 1;
   
   memcpy( &sender_handle, buf + curr_pos, sender_len );
   curr_pos += sender_len;

   number_dest = buf[curr_pos];
   curr_pos += 1;
   
   while (number_dest > 0) {
      
      dest_len = buf[curr_pos];
      curr_pos += 1;
      
      // Need to clear out dest handle buffer each time.
      memset( &dest_handle, '\0', 100 );
      memcpy( &dest_handle, buf + curr_pos, dest_len);
      curr_pos += dest_len;

      if ( handle_exists( server, dest_handle ) != 0) {
         //printf("Handle doesn't exist.\n");
         return_bad_handle( server, client, dest_len, dest_handle );

      } else {
         //printf("Handle EXISTS.\n");
         forward_message( server, buf, dest_handle );
      }
      
      number_dest -= 1;
   }

}

void return_bad_handle (struct server_info * server, struct client_ptr * client, uint8_t dest_len, uint8_t dest_handle[]) {
   
   uint8_t packet[MAXBUF];
   uint16_t pack_len = 4;

   pack_len += pack_handle(packet, 3, (char *) dest_handle);

   packet[0] = htons(pack_len) >> 8;
   packet[1] = htons(pack_len);
   packet[2] = 7;

   wrapped_send(client->client_socket , packet, pack_len, 0);

}

void forward_message (struct server_info * server, uint8_t buf[], uint8_t dest_handle[]) {
   
   //struct chat_header * c_hdr = (struct chat_header *) buf;
   uint32_t dest_socket = get_socket_of_handle( server, dest_handle);
   uint16_t l = ntohs( (buf[0] << 8) + buf[1] );

   if (dest_socket == -1) {
      perror("get_socket_of_handle call");
      exit(EXIT_FAILURE);
   }

   //printf("Dest Handle: %s\n\tDest Socket: %d\n", dest_handle, dest_socket);
   
   wrapped_send(dest_socket, buf, l, 0); //ntohs(c_hdr->packet_len), 0);
}

uint32_t get_socket_of_handle (struct server_info * server, uint8_t dest_handle[]) {
   
   struct client_ptr * temp = server->clients;

   while (temp != NULL) {
      
      if (strcmp( (char *) dest_handle, (char *) &temp->client_handle ) == 0) {
         return temp->client_socket;
      }

      temp = temp->next_client;
   }

   return -1;

}

void flag_eight(struct server_info * server, struct client_ptr * client, uint8_t buf[]) {
  
   uint8_t packet[3];

   packet[0] = htons(3) >> 8;
   packet[1] = htons(3);
   packet[2] = 9;

   wrapped_send(client->client_socket, packet, 3, 0);
}

void flag_ten(struct server_info * server, struct client_ptr * client, uint8_t buf[]) {

   send_number_of_clients(server, client);
   send_all_handles(server, client);
   send_list_finished(server, client);

}

void send_number_of_clients(struct server_info * server, struct client_ptr * client) {
   
   uint8_t packet[MAXBUF];
   uint16_t pack_len = 7;

   packet[3] = htonl(server->number_clients) >> 24;
   packet[4] = htonl(server->number_clients) >> 16;
   packet[5] = htonl(server->number_clients) >> 8;
   packet[6] = htonl(server->number_clients);
   
   packet[0] = htons(pack_len) >> 8;
   packet[1] = htons(pack_len);
   packet[2] = 11;

   wrapped_send(client->client_socket , packet, pack_len, 0);
   
}

void send_all_handles(struct server_info * server, struct client_ptr * client) {
   
   struct client_ptr * temp = server->clients;

   while (temp != NULL) {
      uint8_t packet[MAXBUF];
      uint16_t pack_len = 3;

      pack_len += pack_handle(packet, pack_len, (char *) temp->client_handle);
      packet[0] = htons(pack_len + 1) >> 8;
      packet[1] = htons(pack_len + 1);
      packet[2] = 12;

      wrapped_send(client->client_socket, packet, pack_len + 1, 0);
      temp = temp->next_client;
   }
}

void send_list_finished(struct server_info * server, struct client_ptr * client) {
   
   uint8_t packet[MAXBUF];
   uint16_t pack_len = 3;

   packet[0] = htons(pack_len) >> 8;
   packet[1] = htons(pack_len);
   packet[2] = 13;

   wrapped_send(client->client_socket , packet, pack_len, 0); 
   
}

/*
 *
 * From this point below, we have helper functions that are very specific
 * to this project. Other helper functions are located in helper.c
 *
 */
uint8_t handle_exists (struct server_info * server, uint8_t handle[]) {
   
   struct client_ptr * temp = server->clients;
   
   while (temp != NULL) {
      
      //printf("String 1: %s\nString 2: %s\n", handle, temp->client_handle);
      
      if (strcmp( (char *) handle, (char *) &temp->client_handle) == 0) {
         return 0; // Handle matches.
      }
      temp = temp->next_client;
   }
   
   return 1;  // Handle doesn't match.
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

