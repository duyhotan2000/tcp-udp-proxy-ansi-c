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

enum conn_state {
  c_busy,  /* Busy; waiting for incoming data or for a write to complete. */
  c_done,  /* Done; read incoming data or write finished. */
  c_stop,  /* Stopped. */
  c_dead
};

/* Session states. */
enum session_state
{
	s_parse_udp,
	s_parse_tcp,
    s_proxy_udp,
	s_proxy_tcp,
	s_connect_tcp,
	s_prepare_proxy_tcp,
    s_race,         /* Race data. */
    s_shutdown,     /* Tear down session. */
    s_dead          /* Dead. Safe to free now. */
};

/* Server states. */
enum server_global_state
{
	ss_proxy,/* Dead. Safe to free now. */
	ss_parse,
	ss_race
};

typedef struct proxy_config {
    struct sockaddr_in tcp_server_addr;
    struct sockaddr_in tcp_client_addr;
    struct sockaddr_in tcp_server_sequence_addr;
    struct sockaddr_in tcp_proxy_server_addr;
    struct sockaddr_in udp_server_addr;
    struct sockaddr_in *udp_client_addr;
    uint8_t numOfUDPClient;
    unsigned int ntimeGetSC;
    struct tm *timeGetFile;
    unsigned int ntimeGetFile;
    struct tm *timeGetSC;
    char *filePath;
    char *filter[256];
} proxy_config_t;

typedef struct server_ctx
{
  uv_tcp_t tcp_handle;
  uv_udp_t udp_handle;
  uv_loop_t *loop;
  proxy_config_t* cf;
} server_ctx_t;

typedef struct conn
{
    unsigned char rdstate;
    unsigned char wrstate;
    struct client_ctx_t *client;  /* Backlink to owning client context. */
    ssize_t result;
    union
    {
        uv_handle_t handle;
        uv_stream_t stream;
        uv_tcp_t tcp;
        uv_udp_t udp;
    }
    handle;
    uv_write_t write_req;
    uv_udp_send_t *udp_send_req;
    /* We only need one of these at a time so make them share memory. */
    union
    {
        uv_connect_t connect_req;
        uv_req_t req;
        struct sockaddr_in6 addr6;
        struct sockaddr_in addr4;
        struct sockaddr addr;
        char *buf;  /* Scratch space. Used to read data into. */
    } t;
    uint32_t cnt;
    uint32_t total;
    struct sockaddr_in *addr4_list;
    uint32_t naddr4_list;
} conn_t;

typedef struct client_ctx_t
{
    unsigned int state;
    server_ctx_t *sx;  /* Backlink to owning server context. */
    conn_t incoming;  /* Connection with the client. */
    conn_t outgoing;  /* Connection with the server. */
    proxy_config_t *cf;
} client_ctx_t;

typedef struct server_state
{
    proxy_config_t config;
    server_ctx_t *udp_server;
    server_ctx_t *tcp_server;
    uv_timer_t timerSC;
    uv_timer_t timerGetFile;
    char *fileBuf;
    uint32_t fileLen;
    uv_loop_t *loop;
    int state;
    unsigned int indexTimeGetFile;
    unsigned int indexTimeGetSC;
    uv_fs_t fs_req;
} server_state_t;

typedef struct m_udp_header
{
    unsigned char seqNum[3];
    unsigned char marketID[1];
    unsigned char msgCount[1];
} m_udp_header_t;

typedef struct m_udp_broadcast
{
    unsigned char type[2];
    unsigned char code[1];
    unsigned char timestamp[3];
} m_udp_broadcast_t;

typedef struct m_tcp_header
{
    unsigned char length[2];
    unsigned char seqNum[4];
    unsigned char ackSeqNum[4];
    unsigned char opcode[2];
    unsigned char linkID[2];
} m_tcp_header_t;

typedef struct m_tcp_retran_reply
{
    unsigned char type[2];
    unsigned char firm[4];
    unsigned char marketID[1];
    unsigned char lastSeqNum[3];
    unsigned char seqNum[3];
    unsigned char msgCount[1];
} m_tcp_retran_reply_t;

enum states
{
    proxy,
    race,
    scSent
} STATE;

/* server.c */
int server_run(const proxy_config_t *cf);
void do_next(client_ctx_t *cx);

/* ASSERT() is for debug checks, CHECK() for run-time sanity checks.
 * DEBUG_CHECKS is for expensive debug checks that we only want to
 * enable in debug builds but still want type-checked by the compiler
 * in release builds.
 */
#if defined(NDEBUG)
# define ASSERT(exp)
# define CHECK(exp)   do { if (!(exp)) abort(); } while (0)
# define DEBUG_CHECKS (0)
#else
# define ASSERT(exp)  assert(exp)
# define CHECK(exp)   assert(exp)
# define DEBUG_CHECKS (1)
#endif

#define UNREACHABLE() CHECK(!"Unreachable code reached.")

#define CONTAINER_OF(ptr, type, field)                                        \
  ((type *) ((char *) (ptr) - ((char *) &((type *) 0)->field)))

#endif /* TYPE_H_ */
