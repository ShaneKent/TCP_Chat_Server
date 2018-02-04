#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "helper.h"

void remove_element(uint32_t array[], uint32_t size, uint32_t remove) {
   
   uint8_t passed = 0;
   
   uint32_t i;
   for (i = 0; i < size - 1; i++) {
      if (array[i] == remove)
         passed = 1;

      if (passed == 1)
         array[i] = array[i + 1];
   }

   array = (uint32_t *) realloc(array, sizeof(uint32_t) * (size - 1));
}

void print_buffer(uint8_t buf[], int size) {
   
   printf("\tBuffer: ");
   
   int i;
   for (i = 0; i < size; i++) {
      printf("%02x ", buf[i]);
   }
   
   printf("\n");

}

ssize_t wrapped_send(int sockfd, const void * buf, size_t len, int flags) {

   ssize_t sent;
   uint8_t * temp = (uint8_t *) buf;

   while (len > 0) {
      sent = send(sockfd, buf, len, flags);

      if (sent < 0) {
         perror("send call");
         exit(EXIT_FAILURE);
      }

      temp += sent;
      len -= sent;
   }

   return sent;
}

/*
 * Location is the spot where length of handle will be stored.
 */
uint8_t pack_handle(uint8_t buf[], uint16_t location, char * handle) {

   uint8_t len = 0;

   while (handle[len] != '\0') {
      buf[location + 1 + len] = handle[len];
      len += 1;
   }

   buf[location] = len;

   return len;
   
}

uint8_t pack_text(uint8_t buf[], uint16_t location, char * text) {
   uint8_t len = 0;

   while (text[len] != '\0' && text[len] != '\n') {
      buf[location + len] = text[len];
      len += 1;
   }
  
   return len;
}

uint16_t length_string(char * string) {

   uint16_t len = 0;
   
   while (string[len] != '\0')
      len += 1;

   return len;

}






