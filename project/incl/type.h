/*
 * Copyright (C) 2017 by DXO
 * $Id: type.h Sun Sep 3 00:08:25 GMT+07:00 2017 ctdao $
 * Contact: DXO Team
 */

#ifndef TYPE_H_
#define TYPE_H_

#include "uv.h"
#include "logger.h"

#include <assert.h>
#include <netinet/in.h>  /* sockaddr_in, sockaddr_in6 */
#include <stddef.h>      /* size_t, ssize_t */
#include <stdint.h>
#include <sys/socket.h>  /* sockaddr */

struct client_ctx;

typedef struct {
  const char *bind_host;
  unsigned short bind_port;
  uint8_t log_level;
} server_config;

typedef struct {
  uv_tcp_t tcp_handle;
  uv_loop_t *loop;
} server_ctx;

typedef struct {
  unsigned char rdstate;
  unsigned char wrstate;
  struct client_ctx *client;  /* Backlink to owning client context. */
  ssize_t result;
  union {
    uv_handle_t handle;
    uv_stream_t stream;
    uv_tcp_t tcp;
    uv_udp_t udp;
  } handle;
  uv_write_t write_req;
  /* We only need one of these at a time so make them share memory. */
  union {
    uv_getaddrinfo_t addrinfo_req;
    uv_connect_t connect_req;
    uv_req_t req;
    struct sockaddr_in6 addr6;
    struct sockaddr_in addr4;
    struct sockaddr addr;
    char buf[2048];  /* Scratch space. Used to read data into. */
  } t;
} conn;

typedef struct client_ctx {
  unsigned int state;
  server_ctx *sx;  /* Backlink to owning server context. */
  conn incoming;  /* Connection with the SOCKS client. */
  conn outgoing;  /* Connection with upstream. */
} client_ctx;

typedef struct {
  uv_getaddrinfo_t getaddrinfo_req;
  server_config config;
  server_ctx *servers;
  uv_loop_t *loop;
} server_state;

/* server.c */
int server_run(const server_config *cf, uv_loop_t *loop);

/* client.c */
void client_finish_init(server_ctx *sx, client_ctx *cx);

#endif /* TYPE_H_ */

