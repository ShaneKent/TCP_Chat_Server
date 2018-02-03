#ifndef SERVER

#define SERVER

int main (int argc, char * argv[]);
int check_arguments (int argc, char * argv[]);

void run (uint32_t server_socket);
void set_and_select_file_descriptors (fd_set * rfds, uint32_t fds[], uint32_t num_fds);

void parse_client_message(uint32_t client_socket, uint8_t buf[], fd_set * rfds, uint32_t * fds, uint32_t * num_fds);

struct server_info {
   fd_set rfds;
   uint32_t server_socket;
   uint32_t number_clients;
   struct client_ptr * clients; // Treat this like a linked list.
};

struct client_ptr {
   uint32_t client_socket;
   uint8_t client_handle[100];      // may not want to hard code 100...
   struct client_ptr * next_client; // points to the next client.
};

/*
struct chat_header {
   uint16_t packet_len;
   uint8_t  flag;
} __attribute__((packed));

struct handle_info {
   uint32_t socket_num;
   uint8_t handle_len;
   uint8_t * handle;
} __attribute__((packed));
*/
#endif
