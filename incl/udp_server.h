#include "type.h"
#include <netinet/in.h>  /* INET6_ADDRSTRLEN */
#include <stdlib.h>
#include <string.h>


int udp_server_run(server_state_t *state);
void udp_proxy_init(server_state_t *s);
void udp_proxy_run(server_ctx_t *sx, client_ctx_t *cx);
