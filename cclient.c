#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "cclient.h"
#include "networks.h"
#include "helper.h"

#define STDIO_MAX 1400
#define MAXBUF 3257

int main (int argc, char * argv[]) {

   uint32_t socket_number = 0;

   check_arguments(argc, argv);
   socket_number = (uint32_t) tcpClientSetup(argv[2], argv[3], 0);

   run(argv[1], socket_number);
   
   return EXIT_SUCCESS;
}

void check_arguments(int argc, char * argv[]) {

   if (argc != 4) {
      fprintf(stderr, "Usage: %s handle server-name server-port\n", argv[0]);
      exit(EXIT_FAILURE);
   }

   int i = 0;
   while (argv[1][i] != '\0')
      i += 1;
   
   if (i > 100) {
      printf("Invalid handle, handle longer than 100 characters: %s\n", argv[1]);
      exit(EXIT_SUCCESS);
   }

   return;
}

void run (char * handle, uint32_t socket_number) {

   struct client_info client;
   client.server_socket = socket_number;
   client.number_blocked = 0;
   client.blocked = NULL;
   strcpy( (char *) client.handle, handle);
   
   initiate_good_bad_handle(&client);

   while (1) {

      set_and_select_file_descriptors(&client);

      if (FD_ISSET(STDIN_FILENO, &client.rfds)) {
            
         parse_stdin(&client);
         printf("$: ");
         fflush(stdout);

      } else if (FD_ISSET(client.server_socket, &client.rfds)) {
         
         message_ready(&client);
         printf("$: ");
         fflush(stdout);

      }
   }

}

void initiate_good_bad_handle(struct client_info * client) {
   
   uint32_t len;
   uint8_t buf[MAXBUF];
   struct chat_header * c_hdr;

   initialize_packet(client);
   
   len = recv(client->server_socket, buf, MAXBUF, 0);

   if (len < 0) {
      perror("recv call");
      exit(EXIT_FAILURE);
   }

   c_hdr = (struct chat_header *) buf;

   if (c_hdr->flag == 3) {
      printf("Handle already in use: %s\n", client->handle);
      exit(EXIT_FAILURE);
   }

   printf("$: ");
   fflush(stdout);

}

void initialize_packet(struct client_info * client) {

   uint8_t len = 0;
   uint16_t packet_len = 3;
   uint8_t buf[MAXBUF];

   len = pack_handle( buf, packet_len, (char *) client->handle );
   packet_len += 1 + len;
 
   buf[0] = htons(packet_len);
   buf[1] = htons(packet_len) >> 8;
   buf[2] = 1;

   wrapped_send(client->server_socket, buf, packet_len, 0);
}

void set_and_select_file_descriptors (struct client_info * client) {

   int max_fd = 0;
   struct timeval tv;

   tv.tv_sec = 0;
   tv.tv_usec = 500000;
   FD_ZERO( &client->rfds );
   FD_SET( STDIN_FILENO, &client->rfds );
   FD_SET( client->server_socket, &client->rfds );

   if ( client->server_socket > max_fd )
      max_fd = client->server_socket;
   
   select(max_fd + 1, (fd_set *) &client->rfds, (fd_set *) 0, (fd_set *) 0, &tv);

}

// This function is VERY similar to client_ready in server.c
void message_ready (struct client_info * client) {
   
   uint8_t buf[MAXBUF];
   uint8_t * ptr;
   struct chat_header * c_hdr;
   uint32_t len = recv(client->server_socket, buf, MAXBUF, 0);
   uint32_t temp;

   check_recv_len(client->server_socket, len);
   
   ptr = (uint8_t *) buf;

   while (len > 0) {

      c_hdr = (struct chat_header *) ptr;

      if (len < ntohs(c_hdr->packet_len)) {
         temp = len;
         len += recv(client->server_socket, ptr + temp, MAXBUF, 0);
         check_recv_len(client->server_socket, len);
      }

      if (c_hdr->flag == 5) {
         flag_5(ptr);
      } else if (c_hdr->flag == 7) { 
         flag_7(ptr);
      } else if (c_hdr->flag == 9) {
         flag_9(client);
      } else if (c_hdr->flag == 11) {
         flag_11(ptr);
      } else if (c_hdr->flag == 12) {
         flag_12(ptr);
      } else if (c_hdr->flag == 13) {
         //printf("Flag 13\n");
      }

      ptr += ntohs(c_hdr->packet_len);
      len -= ntohs(c_hdr->packet_len);
   }
    
}

void check_recv_len(uint32_t socket, uint32_t len) {
   
   if (len == -1) {
      perror("recv call");
      exit(EXIT_FAILURE);
   } else if (len == 0) {
      // OOPS! Looks like you're out of luck, silly client...
      close(socket);

      printf("\rServer Terminated\n");
      exit(EXIT_SUCCESS);
   }

}

void flag_5 (uint8_t packet[]) {

   struct chat_header * c_hdr = (struct chat_header *) packet;
   uint16_t curr_pos = 3;
   uint8_t sender_handle[100] = {0};
   uint8_t sender_len = packet[curr_pos];
   uint8_t number_dest;
   uint8_t dest_len;
   uint8_t text[200] = {0};

   curr_pos += 1;

   memset( &sender_handle, '\0', 100);
   memcpy( &sender_handle, packet + curr_pos, sender_len );
   curr_pos += sender_len;
   
   number_dest = packet[curr_pos];
   curr_pos += 1;

   while (number_dest > 0) {
      dest_len = packet[curr_pos];
      curr_pos += 1 + dest_len;

      number_dest -= 1;
   }

   memcpy( &text, packet + curr_pos, ntohs(c_hdr->packet_len) - curr_pos);
   printf("\n%s: %s\n", sender_handle, text);

}

void flag_7 (uint8_t packet[]) {
   
   //struct chat_header * c_hdr = (struct chat_header *) packet;
   uint16_t curr_pos = 3;

   uint8_t dest_handle[100] = {0};
   uint8_t dest_len = packet[curr_pos];
   curr_pos += 1;

   memcpy( &dest_handle, packet + curr_pos, dest_len );
   
   printf("\rClient with handle %s does not exist\n", dest_handle);

}

void flag_9 (struct client_info * client) {

   struct blocked_client * curr, * next;
   
   curr = client->blocked;
   while (curr != NULL) {
      next = curr->next_blocked;
      free(curr);
      curr = next;
   }
   
   client->blocked = NULL;

   printf("\r");

   close(client->server_socket);

   exit(EXIT_SUCCESS);

}

void flag_11 (uint8_t packet[]) {
   
   uint32_t number_handles;

   //print_buffer(packet, 7);

   number_handles = packet[3] + (packet[4] << 8) + (packet[5] << 16) + (packet[6] << 24);
   number_handles = ntohl(number_handles);
   printf("\rNumber of clients: %d\n", number_handles);

}

void flag_12 (uint8_t packet[]) {
   
   //struct chat_header * c_hdr = (struct chat_header *) packet;
   uint16_t curr_pos = 3;

   uint8_t handle[100] = {0};
   uint8_t len = packet[curr_pos];
   curr_pos += 1;

   //print_buffer(packet, ntohs(c_hdr->packet_len));

   memcpy( &handle, packet + curr_pos, len);
   
   printf("\r    %s\n", handle);

}

void parse_stdin(struct client_info * client) {
   
   char * tok;
   uint8_t buf[STDIO_MAX];

   fgets( (char *) buf, MAXBUF, stdin);

   tok = strtok( (char *) buf, " ");
   
   if (strcmp(tok, "%M\n") == 0 || strcmp(tok, "%m\n") == 0) {
      printf("Sending a message requires at least one destination.\n");
   } else if (strcmp(tok, "%M") == 0 || strcmp(tok, "%m") == 0) {
      parse_m_command(client);
   } else if (strcmp(tok, "%B\n") == 0 || strcmp(tok, "%b\n") == 0) {
      list_blocked_clients(client);
   } else if (strcmp(tok, "%B") == 0 || strcmp(tok, "%b") == 0) {
      block_client(client, tok);
   } else if (strcmp(tok, "%U\n") == 0 || strcmp(tok, "%u\n") == 0) {
      printf("Unblock failed, no handle provided.\n");
   } else if (strcmp(tok, "%U") == 0 || strcmp(tok, "%u") == 0) {
      unblock_client(client, tok);
   } else if (strcmp(tok, "%L\n") == 0 || strcmp(tok, "%l\n") == 0) {
      ask_for_handles(client);
   } else if (strcmp(tok, "%L") == 0 || strcmp(tok, "%l") == 0) {
      ask_for_handles(client);
   } else if (strcmp(tok, "%E\n") == 0 || strcmp(tok, "%e\n") == 0) {
      send_exit_request(client); 
   } else if (strcmp(tok, "%E") == 0 || strcmp(tok, "%e") == 0) {
      send_exit_request(client);
   } else {
      printf("Invalid command\n");
   }


}

/*
 * This function is a little long, but I tried to keep the bulk of the code
 *  to be specific to this parsing functionality. Accepting the %M on the
 *  command line is a bit tedious because there are a bunch of edge cases.
 */
void parse_m_command (struct client_info * client) {

   uint8_t handle_len;
   uint16_t packet_len = 3;               // chat header length
   uint8_t packet[MAXBUF];
   char * tok = strtok(NULL, " ");        // Could be either num_handles or destination_handle
   uint8_t num_handles = (uint8_t) atoi(tok);
   
   memset(packet, 0, MAXBUF);

   packet[2] = 5;
   handle_len = pack_handle(packet, packet_len, (char *) client->handle);
   packet_len += 1 + handle_len;          // handle len and len byte.

   if (tok == NULL || *tok == '\n') {
      printf("Sending a message requires at least one destination.\n");
      return;
   }
   
   if (num_handles > 9) {
      printf("Messages can only be sent to less than 10 handles at a time.\n");
      return;
   } else if (num_handles == 0) {         // This is a special case, no num provided.
      num_handles = 1;
      
      packet[packet_len] = num_handles;
      packet_len += 1;                    // num handles byte

      handle_len = pack_handle(packet, packet_len, tok);

      if (tok[handle_len - 1] == '\n') {
         packet[packet_len + handle_len] = '\0';
      }

      packet_len += 1 + handle_len;       // handle len and len byte.
   } else {                               // Any number from 1-9
      packet[packet_len] = num_handles;
      packet_len += 1;

      uint8_t i;
      for (i = 0; i < num_handles; i++) {
         tok = strtok(NULL, " ");
         if (tok == '\0') {
            printf("Provide proper number of destination handles.\n"); return;
         }

         handle_len = pack_handle(packet, packet_len, tok);
         if (tok[handle_len - 1] == '\n') {
            packet[packet_len + handle_len] = '\0';
         }
         
         packet_len += 1 + handle_len;
      }
   }
   
   tok = strtok(NULL, "");
   if (tok == NULL)
      pack_text_and_send(packet, packet_len, "\n"/*tok*/, client->server_socket);
   else
      pack_text_and_send(packet, packet_len, tok, client->server_socket);
}

void pack_text_and_send(uint8_t packet[], uint16_t packet_len, char * tok, uint32_t socket_number) {

   uint16_t text_len = length_string(tok);
   uint8_t num_packets = (text_len / 200) + 1;
   uint16_t length_to_pack;
   uint8_t packed_len;
   uint8_t text[200];
   uint8_t p;

   if (text_len % 200 == 0) num_packets -= 1;

   for (p = 0; p < num_packets; p++) {

      if (text_len > 200)
         length_to_pack = 200;
      else
         length_to_pack = text_len;
      
      memset(text, '0', sizeof(uint8_t));
      strncpy((char *) text, tok, length_to_pack);

      tok = tok + length_to_pack;

      packed_len = pack_handle(packet, packet_len, (char *) text);
      
      packet[0] = htons(packet_len + packed_len);
      packet[1] = htons(packet_len + packed_len) >> 8;
     
      wrapped_send(socket_number, packet, packet_len + packed_len, 0);
      
      text_len -= length_to_pack;
   
   }

}

void list_blocked_clients(struct client_info * client) {

   struct blocked_client * temp = client->blocked;
   uint16_t i = 0;

   printf("Blocked:");
   while (temp != NULL) {
      if (i == client->number_blocked - 1)
         printf(" %s", temp->client_handle);
      else
         printf(" %s,", temp->client_handle);

      i += 1;
      temp = temp->next_blocked;
   }
   printf("\n");

}

void block_client(struct client_info * client, char * tok) {
   
   struct blocked_client * new_block;
   struct blocked_client * temp;
   uint16_t len_tok;
   
   tok = strtok(NULL, "\n");
   if (tok == NULL) {
      list_blocked_clients(client); return;
   } else if (check_if_blocked(client, tok) == 0) {
      printf("Block failed, handle %s is already blocked.\n", tok); return;
   }
   
   len_tok = length_string(tok);
   if (len_tok > 100)
      printf("Block failed, provided handle is greater than 100 characters.\n");
   
   // Free this.
   new_block = (struct blocked_client *) malloc(sizeof(struct blocked_client));
   strcpy((char *) new_block->client_handle, tok);
   new_block->next_blocked = NULL;

   if (client->blocked == NULL) {
      client->blocked = new_block;
   } else {
      temp = client->blocked;
      while (temp->next_blocked != NULL)
         temp = temp->next_blocked;

      temp->next_blocked = new_block;
   }
   client->number_blocked += 1;

   list_blocked_clients(client);
}

// This is based off of delete_client in server.c
void unblock_client (struct client_info * client, char * tok) {
   /*
    * There are 2 cases:
    *  (1) Remove the first node.
    *  (2) Remove some other node.
    */
   
   struct blocked_client * b = client->blocked;
   struct blocked_client * temp;
   
   tok = strtok(NULL, "\n");
   if (tok == NULL) {
      printf("Unblock failed, no handle provided.\n");
      return;
   } else if (check_if_blocked(client, tok) == 1) {
      printf("Unblock failed, handle %s is not blocked.\n", tok);
      return;
   }

   if (b == NULL) {
      printf("Unblock failed, handle %s is not blocked.\n", tok);
      return;
   }

   // First
   if ( strcmp( tok, (char *) b->client_handle ) == 0) {
      client->blocked = b->next_blocked;
      client->number_blocked -= 1;
      
      printf("Handle %s unblocked.\n", tok);
      free(b);
      return;
   }

   //
   while (b->next_blocked != NULL) {
      if ( strcmp( tok, (char *) b->next_blocked->client_handle ) == 0) {
         temp = b->next_blocked;
         b->next_blocked = b->next_blocked->next_blocked;
         client->number_blocked -= 1;
         printf("Handle %s unblocked.\n", tok);
         free(temp);
         return;
      }
      b = b->next_blocked;
   }

   printf("\t\t Got to the end.\n");
}

uint8_t check_if_blocked (struct client_info * client, char * handle) {
   
   struct blocked_client * temp = client->blocked;
   
   while (temp != NULL) {
      if (strcmp( handle, (char *) temp->client_handle ) == 0)
         return 0; // handle IS blocked.
      temp = temp->next_blocked;
   }
   
   return 1;
}

void ask_for_handles(struct client_info * client) {

   uint8_t len = 0;
   uint16_t packet_len = 3;
   uint8_t buf[MAXBUF];

   len = pack_handle( buf, packet_len, (char *) client->handle );
   packet_len += 1 + len;
 
   buf[0] = htons(packet_len);
   buf[1] = htons(packet_len) >> 8;
   buf[2] = 10;

   wrapped_send(client->server_socket, buf, packet_len, 0);

}

void send_exit_request(struct client_info * client) {
   
   uint8_t len = 0;
   uint16_t packet_len = 3;
   uint8_t buf[MAXBUF];

   len = pack_handle( buf, packet_len, (char *) client->handle );
   packet_len += 1 + len;
 
   buf[0] = htons(packet_len);
   buf[1] = htons(packet_len) >> 8;
   buf[2] = 8;

   wrapped_send(client->server_socket, buf, packet_len, 0);

}




