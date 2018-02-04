#ifndef CCLIENT

#define CCLIENT

struct client_info {
   fd_set rfds;
   uint32_t server_socket;
   uint8_t handle[100];
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

void message_ready (struct client_info * client);

void parse_stdin(struct client_info * client);
void parse_m_command(struct client_info * client);
//void parse_m_command(char * handle, uint32_t socket_number);
void pack_text_and_send(uint8_t packet[], uint16_t packet_len, char * tok, uint32_t socket_number);

#endif
