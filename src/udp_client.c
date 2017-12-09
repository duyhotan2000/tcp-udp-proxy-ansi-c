/*
 * Copyright (C) 2017 by DXO
 * $Id: client.c Sun Sep 3 00:42:37 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */

#include "type.h"
#include "util.h"
#include "udp_client.h"
#include <errno.h>
#include <handle_sc.h>
#include <stdlib.h>
#include <string.h>

void udp_proxy_run(server_ctx_t *sx, client_ctx_t *cx)
{
	conn_t *incoming = &cx->incoming; // server side
	conn_t *outgoing = &cx->outgoing; // client side
	cx->state = s_proxy_udp;
	incoming->udp_send_req = 0; // not used
	incoming->t.buf = 0;
	outgoing->udp_send_req = 0;
	outgoing->t.buf = 0; // not used


	incoming->client = cx;
	incoming->rdstate = c_stop;
	incoming->wrstate = c_stop;
	incoming->result = 0;

	outgoing->client = cx;
	outgoing->rdstate = c_stop;
	outgoing->wrstate = c_stop;
	outgoing->result = 0;
	outgoing->addr4_list = sx->cf->udp_client_addr;
	outgoing->naddr4_list = sx->cf->numOfUDPClient;
	outgoing->udp_send_req = (uv_udp_send_t*)xmalloc(sizeof(uv_udp_send_t)*outgoing->naddr4_list);

	uv_udp_init(sx->loop, &outgoing->handle.udp);
	udp_recv_data(&cx->incoming);
}

void udp_recv_data(conn_t *c)
{
    logger_info("UDP Packet Recv Start\n");
	ASSERT(c->rdstate == c_stop);
	uv_udp_recv_start(&c->handle.udp, alloc_buffer, udp_packet_received_cb);
	c->rdstate = c_busy;
}

void udp_packet_received_cb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
    conn_t *c;

    logger_info("UDP Packet Received=%d\n", nread);

    c = CONTAINER_OF(handle, conn_t, handle);
    ASSERT(c->t.buf == buf->base);
    ASSERT(c->rdstate == c_busy);
    c->rdstate = c_done;
    c->result = nread;

    uv_udp_recv_stop(handle);
    do_next(c->client);
}

void udp_send_data(conn_t *c, const void *data, uint32_t len)
{
    logger_info("UDP Packet Send Start\n");

	ASSERT(c->wrstate == c_stop || c->wrstate == c_done);
	c->wrstate = c_busy;
	uv_buf_t buf;
	buf.base = (char*) data;
	buf.len = len;

	uint32_t totalClients = c->naddr4_list;
	struct sockaddr_in *clientList = c->addr4_list;
	c->cnt = 0;
	c->total = totalClients;
	uint32_t i = 0;
	for(; i < totalClients ; ++i)
	{
		(c->udp_send_req+i)->data = c;
		uv_udp_send( c->udp_send_req+i , &(c->handle.udp), &buf, 1, (const struct sockaddr *)(clientList+i), udp_packet_sent_cb);
	}
}

int udp_do_proxy(client_ctx_t *cx)
{
    logger_info("UDP Do Proxy\n");
	conn_t *a = &cx->incoming;
	conn_t *b = &cx->outgoing;

	if(a->result < 0)
	{
		return -1;
	}
	if(b->result < 0)
	{
		return -1;
	}

	if (b->wrstate == c_done) {
		b->wrstate = c_stop;
	}

	if(a->rdstate == c_stop)
	{
		udp_recv_data(a);
	}
	else if (a->rdstate == c_done) {
		udp_send_data(b, a->t.buf, a->result);
		a->rdstate = c_stop;
	}
	return s_proxy_udp;
}

void udp_packet_sent_cb(uv_udp_send_t *req, int status)
{
    if (status)
    {
        logger_error("Send error %s\n", uv_strerror(status));
    }

    conn_t *c = req->data;
    c->cnt++;

	if(c->cnt == c->total)
	{
		ASSERT(c->wrstate == c_busy);
		client_ctx_t *cx = CONTAINER_OF(c, client_ctx_t, outgoing);
		c->wrstate = c_done;
		c->result = status;
		free(cx->incoming.t.buf);
		cx->incoming.t.buf = 0;
		do_next(c->client);
	}
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	conn_t *c;

	c = CONTAINER_OF(handle, conn_t, handle);
	ASSERT(c->rdstate == c_busy);
	c->t.buf = xmalloc(suggested_size);
	buf->base = c->t.buf;
	buf->len = suggested_size;
}

int parse_to_get_sc(client_ctx_t *cx)
{
	logger_info("UDP Parse to get SC\n");
	conn_t *c = &cx->incoming;

	if(c->rdstate == c_stop)
	{
		logger_info("RECVDATA\n");
		udp_do_proxy(cx);
		return s_proxy_udp;
	}
	else if(c->rdstate == c_busy)
	{
		return s_proxy_udp;
	}

    char* clonedBuff = xmalloc(c->result);
    memcpy(clonedBuff, c->t.buf, c->result);

    char msgCount = decodeBase96(clonedBuff[4]);

    const char delim = (char)0x1f;
    char *token;
    token = strtok(clonedBuff+5, &delim);

    /* walk through other tokens */
    while( token != NULL && msgCount-- > 0)
    {
       if(token[0]== 'S' && token[1]== 'C')
       {
           logger_info("FOUND SC ON UDP-----------------\n");
           send_file_set_seq_server(cx);
           free(clonedBuff);
           return s_proxy_udp;
       }

       token = strtok(NULL, &delim);
    }
    free(clonedBuff);

    conn_t *o = &cx->outgoing;
    if(o->wrstate == c_done || o->wrstate == c_stop)
    {
    	udp_do_proxy(cx);
    }
    return s_proxy_udp;
}

