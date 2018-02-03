#ifndef HELPER

#define HELPER

void remove_element (uint32_t array[], uint32_t size, uint32_t remove);
void print_buffer (uint8_t buf[], int size);
ssize_t wrapped_send(int sockfd, const void * buf, size_t len, int flags);
uint8_t pack_handle(uint8_t buf[], uint16_t location, char * handle);
uint16_t length_string(char * string);

#endif
