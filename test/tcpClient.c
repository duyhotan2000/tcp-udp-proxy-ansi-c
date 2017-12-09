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

uv_loop_t *loop;

//TCP
uv_tcp_t server;
struct sockaddr_in addr;
typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

write_req_t write_req;
uv_connect_t connect_req;

void tcp_server_connect_cb(uv_connect_t *req, int status);
void write_data();
void after_write_cb(uv_write_t* req, int status);
void after_read_cb(uv_stream_t* stream, long nread, const uv_buf_t *buf);
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
void tcp_connect();

int main(int argc, char **argv)
{
    loop = uv_default_loop();

    struct sockaddr_in addr;

    // uv_tcp_init(loop, &server);

    uv_ip4_addr("127.0.0.1", atoi(argv[1]), &addr);

 //    int err;
	// err = uv_tcp_connect(&connect_req,
 //                            &server,
 //                            (const struct sockaddr*)&addr,
	// 						tcp_server_connect_cb);

    do
    {
    	sleep(1);
    	uv_tcp_init(loop, &server);
    	uv_tcp_connect(&connect_req,
                        &server,
                        (const struct sockaddr*)&addr,
						tcp_server_connect_cb);
    }
    while(!uv_run(loop, UV_RUN_DEFAULT));
}

void tcp_close_cb(uv_handle_t* req)
{
	printf("*** CLOSE CB ***\n");
	return;
}

void tcp_server_connect_cb(uv_connect_t *req, int status)
{
	if(status)
	{
		printf("*** Connection Error ***: %s\n", uv_strerror(status));
		uv_close((uv_handle_t*)&server, tcp_close_cb);
		return;
	}
	printf("=== Proxy connected ===\n");
	write_data();
}

void write_data()
{
	//uint32_t allocate_size = 50;
	sleep(1);
	char *str = "TCP Client sent data\n";
	write_req.buf.base = (char*)malloc(strlen(str)); 
	memcpy(write_req.buf.base, str, strlen(str));
	write_req.buf.len = strlen(str);
	uv_write(&write_req.req, (uv_stream_t*)&server, &write_req.buf, 1, after_write_cb);
}

void after_write_cb(uv_write_t* req, int status)
{
	if(status)
	{
		printf("*** Sent Error ***: %s\n", uv_strerror(status));
	}
	printf("=== Data sent ===\n");
	free(write_req.buf.base);

	uv_read_start((uv_stream_t *)&server, alloc_buffer, after_read_cb);
}

void after_read_cb(uv_stream_t* stream, long nread, const uv_buf_t *buf)
{
	if(nread < 0)
	{
		printf("*** Read Error ***\n");
		return;
	}
	printf("=== Data received ===: %s\n", buf->base);
	free(buf->base);

	write_data();
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	buf->base = malloc(suggested_size);
	buf->len = suggested_size;
}