#include "type.h"
#include <netinet/in.h>  /* INET6_ADDRSTRLEN */
#include <stdlib.h>
#include <string.h>


int server_run(const server_config *cf, uv_loop_t *loop);
void udp_proxy_init(server_config *udp_config);
void udp_proxy_run(uv_loop_t* loop);
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
void udp_package_received(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
void udp_package_sent(uv_udp_send_t *req, int status);
