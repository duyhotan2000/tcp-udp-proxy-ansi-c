#include <stdint.h>
#include <uv.h>
#include "type.h"
#include "logger.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

void tcp_proxy_run(server_ctx_t *sx, client_ctx_t* cx);
void tcp_read_data(conn_t *c);
void tcp_read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t *buf);
void tcp_write_data(conn_t *c, const void *data, unsigned int len) ;
void tcp_write_cb(uv_write_t *req, int status);
//int tcp_connect(conn_t *c);
//void tcp_connect_cb(uv_connect_t *req, int status);
void tcp_close_conn(conn_t *c);
void tcp_close_conn_cb(uv_handle_t *handle);
void tcp_client_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t *buf);
int tcp_do_proxy_start(client_ctx_t *cx);
int tcp_do_proxy(client_ctx_t *cx);
int tcp_do_forward(const char *who, conn_t *a, conn_t *b);
int tcp_do_remote_connect(client_ctx_t *cx);
int tcp_kill_conn(client_ctx_t *cx);
int tcp_parse_to_get_sc(client_ctx_t *cx);

#endif
