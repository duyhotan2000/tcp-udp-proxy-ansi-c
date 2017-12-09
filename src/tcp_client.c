/*
 * Copyright (C) 2017 by DXO
 * $Id: client.c Sun Sep 3 00:42:37 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */
#include <handle_sc.h>
#include <tcp_client.h>
#include <tcp_server.h>
#include <logger.h>
#include "util.h"
#include "timer.h"

void tcp_proxy_run(server_ctx_t *sx, client_ctx_t* cx)
{
	conn_t *incoming = &cx->incoming;
	conn_t *outgoing = &cx->outgoing;
	cx->state = s_prepare_proxy_tcp;
	cx->cf = sx->cf;
	incoming->udp_send_req = 0; //not used
	incoming->t.buf = 0;
	outgoing->udp_send_req = 0; //not used
	outgoing->t.buf = 0;

	incoming->client = cx;
	incoming->rdstate = c_stop;
	incoming->wrstate = c_stop;
	incoming->result = 0;

	outgoing->client = cx;
	outgoing->rdstate = c_stop;
	outgoing->wrstate = c_stop;
	outgoing->result = 0;
	outgoing->t.addr4 = cx->cf->tcp_server_addr;

	do_next(cx);

	logger_info("Accepted connection\n");
}

void tcp_read_data(conn_t *c)
{
	int err;
    logger_info("TCP Packet Receive Start\n");
	ASSERT(c->rdstate == c_stop);
	err = uv_read_start((uv_stream_t *)&c->handle.tcp, tcp_client_alloc_cb, tcp_read_cb);
	if (err){
		logger_error("Cannot read data from client\n");
		return;
	}
	c->rdstate = c_busy;
}

void tcp_read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t *buf)
{
	conn_t *c;

	c = CONTAINER_OF(handle, conn_t, handle);
	ASSERT(c->t.buf == buf->base);
	ASSERT(c->rdstate == c_busy);
	c->rdstate = c_done;
	c->result = nread;

	uv_read_stop(&c->handle.stream);
	do_next(c->client);
}

void tcp_write_data(conn_t *c, const void *data, unsigned int len) {
  uv_buf_t buf;

  ASSERT(c->wrstate == c_stop || c->wrstate == c_done);
  c->wrstate = c_busy;

  buf.base = (char *) data;
  buf.len = len;

  c->write_req.data = buf.base;
  CHECK(0 == uv_write(&c->write_req,
                      &c->handle.stream,
                      &buf,
                      1,
					  tcp_write_cb));
}

void tcp_write_cb(uv_write_t *req, int status) {
  conn_t *c;

  if (status == UV_ECANCELED) {
    return;  /* Handle has been closed. */
  }

  c = CONTAINER_OF(req, conn_t, write_req);
  ASSERT(c->wrstate == c_busy);
  c->wrstate = c_done;
  c->result = status;
  free(req->data);
  do_next(c->client);
}

//int tcp_connect(conn_t *c) {
//  ASSERT(c->t.addr.sa_family == AF_INET ||
//         c->t.addr.sa_family == AF_INET6);
//  return uv_tcp_connect(&c->t.connect_req,
//                        &c->handle.tcp,
//                        (const struct sockaddr*)&c->t.addr4,
//						tcp_connect_cb);
//}
//
//void tcp_connect_cb(uv_connect_t *req, int status) {
//  conn_t *c;
//
//  if (status == UV_ECANCELED) {
//    return;  /* Handle has been closed. */
//  }
//
//  c = CONTAINER_OF(req, conn_t, t.connect_req);
//  c->result = status;
//  do_next(c->client);
//}

void tcp_close_conn(conn_t *c) {
  ASSERT(c->rdstate != c_dead);
  ASSERT(c->wrstate != c_dead);
  c->rdstate = c_dead;
  c->wrstate = c_dead;
  c->handle.handle.data = c;
  uv_close(&c->handle.handle, tcp_close_conn_cb);
}

void tcp_close_conn_cb(uv_handle_t *handle) {
  conn_t *c;

  c = handle->data;
  do_next(c->client);
}

void tcp_client_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t *buf)
{
	conn_t *c;
	c = CONTAINER_OF(handle, conn_t, handle);
	ASSERT(c->rdstate == c_busy);
	c->t.buf = (char*)xmalloc(suggested_size);
	buf->base = c->t.buf;
	buf->len = suggested_size;
}

int tcp_do_proxy_start(client_ctx_t *cx) {
  conn_t *incoming;
  conn_t *outgoing;

  incoming = &cx->incoming;
  outgoing = &cx->outgoing;
  ASSERT(incoming->rdstate == c_stop);
  ASSERT(incoming->wrstate == c_stop);
  ASSERT(outgoing->rdstate == c_stop);
  ASSERT(outgoing->wrstate == c_stop);

  if (incoming->result < 0) {
	  logger_error("Write error: %s\n", uv_strerror(incoming->result));
    return tcp_kill_conn(cx);
  }

  tcp_read_data(incoming);
  tcp_read_data(outgoing);
  return s_proxy_tcp;
}

int tcp_do_proxy(client_ctx_t *cx)
{
    logger_info("TCP Do proxy\n");

    if (tcp_do_forward("client", &cx->incoming, &cx->outgoing)) {
          return tcp_kill_conn(cx);
        }

    if (tcp_do_forward("upstream", &cx->outgoing, &cx->incoming)) {
      return tcp_kill_conn(cx);
    }

    return s_proxy_tcp;
}

int tcp_do_forward(const char *who, conn_t *a, conn_t *b) {
  if (a->result < 0) {
    if (a->result != UV_EOF) {
    	logger_error("%s error: %s", who, uv_strerror(a->result));
    }
    return -1;
  }

  if (b->result < 0) {
    return -1;
  }

  if (a->wrstate == c_done) {
    a->wrstate = c_stop;
  }

  if (a->wrstate == c_stop) {
    if (b->rdstate == c_stop) {
      tcp_read_data(b);
    } else if (b->rdstate == c_done) {
      tcp_write_data(a, b->t.buf, b->result);
      b->rdstate = c_stop;  /* Triggers the call to conn_read() above. */
    }
  }

  return 0;
}

//

int tcp_kill_conn(client_ctx_t *cx) {
  int new_state = s_dead;

  tcp_close_conn(&cx->incoming);
  tcp_close_conn(&cx->outgoing);
  return new_state;
}

int tcp_parse_to_get_sc(client_ctx_t *cx)
{
	logger_info("TCP Parse to get SC\n");
	conn_t *c = &cx->outgoing;

	if(c->rdstate != c_done)
	{
		logger_info("PARSING TIME\n");
		tcp_do_proxy(cx);
		return s_proxy_tcp;
	}

    char* clonedBuff = (char*)xmalloc(c->result);
    memcpy(clonedBuff, c->t.buf, c->result);

    char msgCount = decodeBase96(clonedBuff[25]);

    const char delim = (char)0x1f;
    char *token;
    token = strtok(clonedBuff+26, &delim);

    /* walk through other tokens */
    while( token != NULL && msgCount-- > 0)
    {
       if(token[0]== 'S' && token[1]== 'C')
       {
           logger_info("FOUND SC ON TCP-----------------\n");
           send_file_set_seq_server(cx);

           free(clonedBuff);
           return s_proxy_tcp;
       }

       token = strtok(NULL, &delim);
    }
    free(clonedBuff);

    tcp_do_proxy(cx);
    return s_proxy_tcp;
}

