#include <tcp_server.h>
#include <tcp_client.h>
#include <logger.h>
#include "parse_config.h"
#include "util.h"

int tcp_server_run(server_state_t *state)
{
	tcp_proxy_init(state);
	server_ctx_t *sx = state->tcp_server;
	sx->loop = state->loop;
	sx->cf = &state->config;

	client_ctx_t *cx = (client_ctx_t*)xmalloc(sizeof(client_ctx_t));
	cx->outgoing.t.addr4 = sx->cf->tcp_server_addr;
	cx->sx = sx;
	cx->cf = &state->config;

    uv_tcp_init(state->loop, &cx->outgoing.handle.tcp);
    uv_tcp_connect(&cx->outgoing.t.connect_req,
                            &cx->outgoing.handle.tcp,
                            (const struct sockaddr*)&cx->outgoing.t.addr4,
							tcp_server_connect_cb);

    return 0;
}

void tcp_proxy_init(server_state_t *state)
{
	state->tcp_server = (server_ctx_t*)xmalloc(sizeof(server_ctx_t));
}

void tcp_server_connect_cb(uv_connect_t* req, int status)
{
	int err;

	if (status) {
		logger_error("Cannot connect to TCP server: %s\n", uv_strerror(status));
		exit(1);
	}

	conn_t *c = CONTAINER_OF(req, conn_t, t.connect_req);
	client_ctx_t* cx = CONTAINER_OF(c, client_ctx_t, outgoing);
	server_ctx_t* sx = cx->sx;
	sx->tcp_handle.data = cx;

	uv_tcp_init(sx->loop, &sx->tcp_handle);
    err = uv_tcp_bind(&sx->tcp_handle, (const struct sockaddr*)&sx->cf->tcp_proxy_server_addr, 0);
    logger_info("TCP Proxy Server Started\n");
    if (err){
      logger_error("Cannot bind TCP Proxy address");
      exit(1);
    }

    const int backlog = 128;
    //proxy_handle.data = (void*)p_config;
    err = uv_listen((uv_stream_t*) &sx->tcp_handle, backlog, tcp_client_connected_cb);

    if (err){
      logger_error("Cannot listen TCP interface");
      exit(1);
    }
}

void tcp_client_connected_cb(uv_stream_t *server, int status)
{
	if (status) {
		logger_error("Client connection error: %s\n", uv_strerror(status));
		exit(1);
	}
	client_ctx_t *cx = server->data;
	server_ctx_t *sx = cx->sx;

	CHECK(0 == uv_tcp_init(sx->loop, &cx->incoming.handle.tcp));
	CHECK(0 == uv_accept(server, (uv_stream_t *)&cx->incoming.handle.stream));

	if(!check_filter_list((uv_tcp_t*)&cx->incoming.handle.stream, sx->cf))
	{
		logger_info("TCP client IP address not accepted\n");
		return;
	}

	tcp_proxy_run(sx,cx);
}

int check_filter_list(uv_tcp_t *stream, proxy_config_t *p_config)
{
    struct sockaddr_storage remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    int namelen = sizeof(remote_addr);
    if (uv_tcp_getpeername(stream, (struct sockaddr *)&remote_addr, &namelen))
    {
        logger_fatal("Unknown address \n");
        return 0;
    }

    char *ip_str = sockaddr_to_str(&remote_addr);
    unsigned int i =0;
    while(p_config->filter[i] != NULL)
    {
    	logger_info("IP address  %d %d %s\n", i, strlen(ip_str), p_config->filter[i]);
        if (strncmp(p_config->filter[i], ip_str, strlen(ip_str)) == 0)
        {
            logger_info("IP address is accepted %s  \n", p_config->filter[i]);
            return 1;
        }
        ++i;
    }

    if (!ip_str)
    {
        logger_fatal("Unknown address type \n");
    }
    free(ip_str);
    return 0;
}
