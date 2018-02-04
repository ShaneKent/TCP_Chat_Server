#ifndef SERVER

#define SERVER

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

// basing this format after the structures I used for prgm1.
struct chat_header {
   uint16_t packet_len;
   uint8_t  flag;
} __attribute__((packed));

struct initialize_packet {
   struct chat_header c_hdr;
   uint8_t sender_handle_len;
   uint8_t * sender_handle;//[100];
} __attribute__((packed));

struct msg_packet {
   struct chat_header c_hdr;
   uint8_t send_handle_len;
   uint8_t * sender_handle;
   uint8_t number_dest;
} __attribute__((packed));

int main (int argc, char * argv[]);
int check_arguments (int argc, char * argv[]);

void run (uint32_t server_socket);
void set_and_select_file_descriptors (struct server_info * server);

void new_client_connected (struct server_info * server);
void delete_client (struct server_info * server, struct client_ptr * client);
void client_ready (struct server_info * server, struct client_ptr * client);

void flag_one (struct server_info * server, struct client_ptr * client, uint8_t buf[]);
void bad_handle (struct server_info * server, struct client_ptr * client);
void good_handle (struct server_info * server, struct client_ptr * client);

void flag_five (struct server_info * server, struct client_ptr * client, uint8_t buf[]);

uint8_t handle_exists (struct server_info * server, uint8_t handle[]);
void print_all_clients(struct server_info * server);

#endif
