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
   server.number_clients
   //fd_set rfds;

   uint8_t buf[MAXBUF];

   uint32_t num_fds = 1;
   uint32_t * fds = (uint32_t *) malloc( sizeof(uint32_t) * num_fds );

   fds[0] = server_socket;

   while (1) {
      
      set_and_select_file_descriptors(&rfds, fds, num_fds);
      
      if (FD_ISSET(server_socket, &rfds)) {
         uint32_t client_socket;
         client_socket = tcpAccept(server_socket, 0);
         
         printf("Client connected on socket: %d\n", client_socket);

         num_fds += 1;
         fds = (uint32_t *) realloc(fds, sizeof(uint32_t) * num_fds );
         fds[num_fds - 1] = client_socket;
      }

      int i;
      for (i = 1; i < num_fds; i++) {

         parse_client_message(fds[i], buf, &rfds, fds, &num_fds);
         
      }

   }
}

void set_and_select_file_descriptors (fd_set * rfds, uint32_t fds[], uint32_t num_fds) {
   
   int i;
   uint32_t max_fd = 0;
   struct timeval tv;

   tv.tv_sec = 1;
   tv.tv_usec = 0;
   FD_ZERO(rfds);

   for (i = 0; i < num_fds; i++) {
      FD_SET(fds[i], rfds);
      if (fds[i] > max_fd) {
         max_fd = fds[i];
      }
   }

   select(max_fd + 1, (fd_set *) rfds, (fd_set *) 0, (fd_set *) 0, &tv);
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






