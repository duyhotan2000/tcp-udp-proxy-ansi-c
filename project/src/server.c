/*
 * Copyright (C) 2017 by DXO
 * $Id: server.c Sun Sep 3 00:20:38 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */

#include "server.h"
#include <netinet/in.h>  /* INET6_ADDRSTRLEN */
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "timer.h"

#ifndef INET6_ADDRSTRLEN
# define INET6_ADDRSTRLEN 63
#endif

uv_udp_t send_socket;
uv_udp_t recv_socket;
uv_udp_send_t send_req;

int server_run(const server_config *cf, uv_loop_t *loop)
{
    logger_trace("%s %d", cf->bind_host, cf->bind_port);

    udp_proxy_run(loop);

    timer_run(loop);

    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}

void udp_proxy_init(server_config* udp_config, uv_loop_t *loop)
{
    uv_udp_init(loop, &recv_socket);
    uv_udp_init(loop, &send_socket);
    struct sockaddr_in recv_addr;
    uv_ip4_addr(udp_config->bind_host, udp_config->bind_port, &recv_addr);
    uv_udp_bind(&recv_socket, (const struct sockaddr *)&recv_addr, UV_UDP_REUSEADDR);
}

void udp_proxy_run(uv_loop_t* loop)
{
    uv_udp_recv_start(&recv_socket, alloc_buffer, udp_package_received_cb);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

void udp_package_received_cb(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
    if (nread <= 0)
    {
        //Read Error or Nothing to read or an Empty UDP package received
        free(buf->base);
        return;
    }

    uv_buf_t clonedBuff;
    alloc_buffer(NULL, nread, &clonedBuff);
    memcpy(clonedBuff.base, buf->base, nread);

    struct sockaddr_in clientList;
    char totalClients = 0;
    getClientListFromFile(&clientList, &totalClients );
    char i = 0;
    for(; i< totalClients ; ++i)
    {
        uv_udp_send(&send_req, &send_socket, &clonedBuff, 1, (const struct sockaddr *)&clientList, udp_package_sent_cb);
    }

    free(buf->base);
    free(clonedBuff.base);
}

void udp_package_sent_cb(uv_udp_send_t *req, int status)
{
    if (status)
    {
        fprintf(stderr, "Send error %s\n", uv_strerror(status));
    }
    else
    {
        fprintf(stderr, "Send Done\n");
    }
}
