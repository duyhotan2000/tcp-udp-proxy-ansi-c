/*
 * udp_client.h
 *
 *  Created on: Sep 16, 2017
 *      Author: duy
 */

#ifndef INCL_UDP_CLIENT_H_
#define INCL_UDP_CLIENT_H_

void udp_proxy_run(server_ctx_t *sx, client_ctx_t *cx);
void forward_udp_to_clients(ssize_t nread, const uv_buf_t *buf);
void udp_packet_sent_cb(uv_udp_send_t *req, int status);
void udp_packet_received_cb(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
int parse_to_get_sc(client_ctx_t *cx);
int udp_do_proxy(client_ctx_t* cx);
void udp_recv_data(conn_t *c);

#endif /* INCL_UDP_CLIENT_H_ */
