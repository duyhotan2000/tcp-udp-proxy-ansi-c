#include <stdint.h>
#include <type.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#ifndef SERVER_H_
#define SERVER_H_


int tcp_server_run(server_state_t *state);
void tcp_proxy_init(server_state_t *state);
void tcp_server_connect_cb(uv_connect_t* req, int status);
void tcp_client_connected_cb(uv_stream_t *server, int status);
int check_filter_list(uv_tcp_t *stream, proxy_config_t *p_config);

#endif
