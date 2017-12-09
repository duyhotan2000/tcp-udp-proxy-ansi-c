/*
 * Copyright (C) 2017 by DXO
 * $Id: server.c Sun Sep 3 00:20:38 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */

#include <netinet/in.h>  /* INET6_ADDRSTRLEN */
#include <stdlib.h>
#include <string.h>
#include <udp_server.h>
#include "timer.h"
#include "util.h"

#ifndef INET6_ADDRSTRLEN
# define INET6_ADDRSTRLEN 63
#endif

int udp_server_run(server_state_t* state)
{
    logger_info("UDP Server Run\n");
	client_ctx_t *udp_cx = xmalloc(sizeof(client_ctx_t));
    udp_proxy_init(state);
    state->udp_server->cf = &state->config;
    state->udp_server->loop = state->loop;

    uv_udp_init(state->loop, &(udp_cx->incoming.handle.udp));

    struct sockaddr_in recv_addr = state->config.udp_server_addr;
    uv_udp_bind(&(udp_cx->incoming.handle.udp), (const struct sockaddr *)&recv_addr, UV_UDP_REUSEADDR);
    udp_cx->cf = &state->config;
    udp_proxy_run(state->udp_server,udp_cx);

    return 0;
}

void udp_proxy_init(server_state_t *state)
{
	state->udp_server = xmalloc(sizeof(server_ctx_t));
}

