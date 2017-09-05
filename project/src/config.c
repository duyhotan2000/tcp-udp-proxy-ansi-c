#include "config.h"
#include "type.h"
#include "logger.h"

void getConfigFromFile(server_config *udp_config)
{
    udp_config->bind_host = "127.0.0.1";
    udp_config->bind_port = 2222;
    udp_config->log_level = LOGGER_LEVEL_INFO;
}

void getClientListFromFile(struct sockaddr_in *address, char *number)
{
    uv_ip4_addr("127.0.0.1", 1111, address);
    *number = 1;
}
