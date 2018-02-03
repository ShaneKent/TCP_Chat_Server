#ifndef CCLIENT

#define CCLIENT

int main (int argc, char * argv[]);
void check_arguments (int argc, char * argv[]);

void run (char * handle, uint32_t socket_number);
void initiate_good_bad_handle(uint8_t buf[], char * handle, uint32_t socket_number);
void initialize_packet(uint8_t buf[], char * handle, uint32_t socket_number);

void set_and_select_file_descriptors (fd_set * rfds, uint32_t fds[], uint32_t num_fds);

void parse_stdin(uint8_t buf[], char * handle, uint32_t socket_number);
void parse_m_command(char * handle, uint32_t socket_number);
void pack_text_and_send(uint8_t packet[], uint16_t packet_len, char * tok, uint32_t socket_number);

#endif
