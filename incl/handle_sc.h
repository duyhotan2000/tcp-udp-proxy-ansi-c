#include "uv.h"

#include <assert.h>
#include <netinet/in.h>  /* sockaddr_in, sockaddr_in6 */
#include <stddef.h>      /* size_t, ssize_t */
#include <stdint.h>
#include <sys/socket.h>  /* sockaddr */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "type.h"

uv_buf_t fileBuf;

void init();
void send_file_set_seq_server(client_ctx_t *cx);
void file_server_on_connect_cb(uv_connect_t *req, int status);
void seq_server_on_connect_cb(uv_connect_t *req, int status);
void done_write_cb(uv_write_t *req, int status);
void on_close_conn_cb(uv_handle_t* handle);
void alloc_buffer_handle_sc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);


