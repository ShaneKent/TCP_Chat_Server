#ifndef CCLIENT

#define CCLIENT

struct client_info {
   fd_set rfds;
   uint32_t server_socket;
   uint8_t handle[100];
   uint8_t number_blocked;
   struct blocked_client * blocked;
};

struct blocked_client {
   uint8_t client_handle[100];
   struct blocked_client * next_blocked;
};

struct chat_header {
   uint16_t packet_len;
   uint8_t  flag;
} __attribute__((packed));

int main (int argc, char * argv[]);
void check_arguments (int argc, char * argv[]);

void run (char * handle, uint32_t socket_number);
void initiate_good_bad_handle(struct client_info * client);
void initialize_packet(struct client_info * client);

void set_and_select_file_descriptors(struct client_info * client);

void message_ready(struct client_info * client);
void check_recv_len(uint32_t socket, uint32_t len);

void flag_5(uint8_t packet[]);
void flag_7(uint8_t packet[]);
void flag_9(struct client_info * client);
void flag_11(uint8_t packet[]);
void flag_12(uint8_t packet[]);

void parse_stdin(struct client_info * client);
void parse_m_command(struct client_info * client);
void pack_text_and_send(uint8_t packet[], uint16_t packet_len, char * tok, uint32_t socket_number);

void list_blocked_clients(struct client_info * client);
void block_client(struct client_info * client, char * tok);
void unblock_client(struct client_info * client, char * tok);
uint8_t check_if_blocked (struct client_info * client, char * handle);

void ask_for_handles(struct client_info * client);

void send_exit_request(struct client_info * client);

#endif
