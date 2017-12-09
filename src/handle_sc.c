#include "uv.h"

#include <assert.h>
#include <handle_sc.h>
#include <netinet/in.h>  /* sockaddr_in, sockaddr_in6 */
#include <stddef.h>      /* size_t, ssize_t */
#include <stdint.h>
#include <sys/socket.h>  /* sockaddr */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <udp_server.h>
#include "type.h"
#include "util.h"

extern uv_loop_t *loop;
extern int sv_global_state;

uv_tcp_t file_socket;
uv_tcp_t seq_socket;
uv_write_t send_file_req;
uv_write_t send_seq_req;
uv_buf_t buf;
size_t oriBufLen;
uv_buf_t seq;
uv_connect_t file_handle;
uv_connect_t seq_handle;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

static char countSentCB;
static client_ctx_t *local_cx;

void init()
{
    uv_tcp_init(loop, &seq_socket);
    uv_tcp_init(loop, &file_socket);
    countSentCB = 0;
}
void send_file_set_seq_server(client_ctx_t *cx)
{
	logger_info("SEND FILE GET SEQ SV-----------------\n");
	local_cx = cx;
	proxy_config_t *cf = cx->cf;
	server_state_t *state = CONTAINER_OF(cf, server_state_t, config);
    init();

    buf.base = state->fileBuf;
    buf.len = state->fileLen;

    oriBufLen = buf.len;

    uv_tcp_connect(&file_handle, &file_socket, (const struct sockaddr*)&cx->cf->tcp_server_addr, file_server_on_connect_cb);

    //Send SetSEQ
    uv_tcp_connect(&seq_handle, &seq_socket, (const struct sockaddr*)&cx->cf->tcp_server_sequence_addr, seq_server_on_connect_cb);
}

void file_server_on_connect_cb(uv_connect_t *req, int status)
{
    if (status < 0)
    {
        logger_error("connect failed error %s\n", uv_err_name(status));
        free(buf.base);
        return;
    }
    logger_info("CONNECTED TO SERVER TO SEND FILE\n");

    buf.len = buf.len - 1 - 4;
    int r = uv_write(&send_file_req, (uv_stream_t*)req->handle, &buf, 1, done_write_cb);

    if(r)
    {
        logger_error("Send error %s\n", uv_strerror(status));
    }
}

void seq_server_on_connect_cb(uv_connect_t *req, int status)
{
    if (status < 0)
    {
        logger_error("connect failed error %s\n", uv_err_name(status));
        return;
    }
    logger_info("CONNECTED TO SERVER TO SEND SEQ\n");
    if(buf.len > 4)
    {
        seq.base = buf.base + (oriBufLen - 1 - 4);
        seq.len = 4;
    }
    else{
    	logger_error("File doesn't have last 4 bytes\n");
    	return;
    }

    int r = uv_write(&send_seq_req, (uv_stream_t*)req->handle, &seq, 1, done_write_cb);

    if(r)
    {
        logger_error("Send error %s\n", uv_strerror(status));
    }
}

void done_write_cb(uv_write_t *req, int status)
{
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
//    write_req_t *wr = (write_req_t*) req;
//    free(wr->buf.base);

    if(++countSentCB == 2)
    {
        //STATE = proxy
    	free(buf.base);
//    	free(seq.base);
    	uv_close((uv_handle_t*)req->handle , on_close_conn_cb);
    	sv_global_state = ss_proxy;
        do_next(local_cx);
    }
}

void on_close_conn_cb(uv_handle_t* handle)
{
    //free(handle);
}

void alloc_buffer_handle_sc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	buf->base = xmalloc(suggested_size);
	buf->len = suggested_size;
}

