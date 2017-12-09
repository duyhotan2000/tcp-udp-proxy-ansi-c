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
#include "parse_config.h"

#define ASSERT(exp)  assert(exp)

#define TCP_PORT_SEVER  1234    //send/receive MSG to/from proxy

#define TCP_PORT_SEQ    1236    //receive Seq

#define TCP_PORT_CTL    9999    //telnet to this port

#define DEFAULT_BACKLOG 128
#define MAX_LENGTH 65535

int64_t counter = 0;
int64_t counter1 = 0;

char udpSC = 0;
char tcpSC = 0;

uv_loop_t *loop;
uv_udp_t send_socket;
uv_prepare_t prep;
uv_check_t check;
uv_idle_t idler;

//TCP
uv_tcp_t server;
uv_tcp_t server1;
uv_tcp_t server2;
uv_tcp_t server3;

uv_tcp_t *proxy_client;

struct sockaddr_in addr;
typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;


union SeqNum {
   unsigned int SeqNum;
   char str[4];
};

proxy_config_t *proxy_config;
char *binary_file;

uv_buf_t buildUdpMessage();
uv_buf_t buildTcpMessage();
void udp_package_received_cb(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
void udp_package_sent_cb(uv_udp_send_t *req, int status);
void timer_check_cb(uv_check_t *handle);
void idle_cb(uv_idle_t *handle);

//TCP
void on_new_connection(uv_stream_t *server, int status);
void on_close(uv_handle_t* handle);
void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
void echo_write(uv_write_t *req, int status);
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
void free_write_req(uv_write_t *req);

int main(int argc, char **argv)
{
    proxy_config = get_config("config.conf");
    if(!proxy_config)
    {
        printf("LOAD_CONFIG_ERR!\n");
        return 1;
    }

    loop = uv_default_loop();

    uv_check_init(loop, &check);
    uv_check_start(&check, timer_check_cb);
    uv_idle_init(loop, &idler);
    uv_idle_start(&idler, idle_cb);

    //TCP
    uv_tcp_init(loop, &server);
    uv_tcp_init(loop, &server1);
    uv_tcp_init(loop, &server2);

    uv_tcp_init(loop, &server1);
    uv_ip4_addr("127.0.0.1", TCP_PORT_SEQ, &addr);
    uv_tcp_bind(&server1, (const struct sockaddr*)&addr, 0);
    server1.data = (void*)TCP_PORT_SEQ;
    uv_listen((uv_stream_t*) &server1, DEFAULT_BACKLOG, on_new_connection);


    uv_tcp_init(loop, &server2);
    uv_ip4_addr("127.0.0.1", TCP_PORT_CTL, &addr);
    uv_tcp_bind(&server2, (const struct sockaddr*)&addr, 0);
    server2.data = (void*)TCP_PORT_CTL;
    uv_listen((uv_stream_t*) &server2, DEFAULT_BACKLOG, on_new_connection);

    uv_tcp_init(loop, &server3);
    uv_ip4_addr("127.0.0.1", TCP_PORT_SEVER, &addr);
    uv_tcp_bind(&server3, (const struct sockaddr*)&addr, 0);
    server3.data = (void*)TCP_PORT_SEVER;
    uv_listen((uv_stream_t*) &server3, DEFAULT_BACKLOG, on_new_connection);

    //UDP
    uv_udp_init(loop, &send_socket);
    struct sockaddr_in broadcast_addr;
    uv_ip4_addr("127.0.0.1", 2001, &broadcast_addr);
    uv_udp_bind(&send_socket, (const struct sockaddr *)&broadcast_addr, 0);
    uv_udp_set_broadcast(&send_socket, 1);

    uv_run(loop, UV_RUN_DEFAULT);
}
void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;
}

void echo_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free_write_req(req);
}
void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
    if (nread > 0) {
        write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
        req->buf = uv_buf_init(buf->base, nread);
        int port = (int)client->data;
        switch(port)
        {
        case TCP_PORT_CTL:
            switch(req->buf.base[0])
            {
            case 't':
                tcpSC = 1;
                break;
            case 'u':
                udpSC = 1;
                break;
            }
            return;
        case TCP_PORT_SEQ:
            fprintf(stderr, "SEQ Received %s\n", req->buf.base);
            binary_file = malloc(MAX_LENGTH);
            long len = getFileToBuffer(proxy_config->filePath, &binary_file);
            binary_file[len] = '\0';
            printf("File content: %s\n", binary_file);
            ASSERT(!memcmp(&req->buf.base[nread-4], &binary_file[len-5], 4));
            break;
        case TCP_PORT_SEVER:
            {
                fprintf(stderr, "Received %s\n", req->buf.base);

                uv_buf_t tcp_msg = buildTcpMessage();
                uv_write((uv_write_t*) req, client, &tcp_msg, 1, echo_write);
                return;
            }
            break;
        default:
            break;
        }

        uv_write((uv_write_t*) req, client, &req->buf, 1, echo_write);
        return;
    }
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, on_close);
    }

    free(buf->base);
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));

    if((int)server->data == TCP_PORT_SEVER)
    {
        proxy_client = client;
    }
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        client->data = server->data;
        uv_read_start((uv_stream_t*) client, alloc_buffer, echo_read);
    }
    else {
        uv_close((uv_handle_t*) client, on_close);
    }
}

void on_close(uv_handle_t* handle) {
    free(handle);
    fprintf(stderr, "Connection close \n");
}

uv_buf_t buildUdpMessage()
{
    uv_buf_t buffer;
    alloc_buffer(NULL, 100, &buffer);
    memset(buffer.base, 0, buffer.len);

    // SeqNum
    buffer.base[0] = 'S';
    buffer.base[1] = 'E';
    buffer.base[2] = 'Q';

    // MarketID
    buffer.base[3] = '@';

    unsigned char count = 2;
    // MsgCount
    buffer.base[4] = count + 32; //encode Mod96

    unsigned short msgIndex = 5;
    char i = 0;
    for(; i<count; ++i)
    {
        buffer.base[msgIndex++] = 0x41;
        buffer.base[msgIndex++] = 0x42;
        buffer.base[msgIndex++] = 0x43;
        buffer.base[msgIndex++] = 0x44;
        buffer.base[msgIndex++] = 0x45;
        buffer.base[msgIndex++] = '_';
        buffer.base[msgIndex++] = 0x1f;
    }

    if(udpSC)
    {
        buffer.base[4] = buffer.base[4] + 1;
        buffer.base[msgIndex++] = 'S';
        buffer.base[msgIndex++] = 'C';
        buffer.base[msgIndex++] = '_';
        buffer.base[msgIndex++] = 0x1f;
        udpSC = 0;
    }

    buffer.base[4] = buffer.base[4] + 1;
    buffer.base[msgIndex++] = 'E';
    buffer.base[msgIndex++] = 'N';
    buffer.base[msgIndex++] = 'D';
    buffer.base[msgIndex++] = '_';

    return buffer;
}

uv_buf_t buildTcpMessage()
{

    uv_buf_t buffer;
    alloc_buffer(NULL, 100, &buffer);
    memset(buffer.base, 0, buffer.len);

    char index=0;
    // Len[2]
    buffer.base[index++] = 'L';
    buffer.base[index++] = 'E';

    //Seq[4]
    buffer.base[index++] = 'S';
    buffer.base[index++] = 'E';
    buffer.base[index++] = 'E';
    buffer.base[index++] = 'Q';

    //ACK[4]
    buffer.base[index++] = 'A';
    buffer.base[index++] = 'C';
    buffer.base[index++] = 'K';
    buffer.base[index++] = 'K';

    //Opcode[2]
    buffer.base[index++] = 'R';
    buffer.base[index++] = 'P';

    //Link[2]
    buffer.base[index++] = 'L';
    buffer.base[index++] = 'K';

    //Content
    //1. MsgType[2]
    buffer.base[index++] = 'R';
    buffer.base[index++] = 'P';

    //2. Firm[2]
    buffer.base[index++] = 'F';
    buffer.base[index++] = 'R';

    //3. MarketID[1]
    buffer.base[index++] = 'M';

    //4. Previous SeqNum[3]
    buffer.base[index++] = 'P';
    buffer.base[index++] = 'R';
    buffer.base[index++] = 'V';

    //5. SeqNum[3]
    buffer.base[index++] = 'S';
    buffer.base[index++] = 'E';
    buffer.base[index++] = 'Q';

    //6. MsgCount[1]
    char count = 2;
    buffer.base[index++] = count + 32; //[25]

    //7. Origin BC MSG
    char i = 0;
    for(; i<count; ++i)
    {
        buffer.base[index++] = 0x41;
        buffer.base[index++] = 0x42;
        buffer.base[index++] = 0x43;
        buffer.base[index++] = '_';
        buffer.base[index++] = 0x1f;
    }

    if(tcpSC)
    {
        buffer.base[25] = buffer.base[25] + 1;
        buffer.base[index++] = 'S';
        buffer.base[index++] = 'C';
        buffer.base[index++] = '_';
        buffer.base[index++] = 0x1f;
        tcpSC = 0;
    }

    buffer.base[25] = buffer.base[25] + 1;
    buffer.base[index++] = 'E';
    buffer.base[index++] = 'N';
    buffer.base[index++] = 'D';
    buffer.base[index++] = '_';

    //7. Origin BC MSG
    buffer.base[index++] = 'E';

    return buffer;
}
void timer_check_cb(uv_check_t *handle)
{
    if(++counter == 10000000)
    {
//        printf("Timer still working\n");
        counter =0;

        uv_udp_send_t *send_req = malloc(sizeof(uv_udp_send_t));
        uv_buf_t udp_broadcast_msg = buildUdpMessage();

        struct sockaddr_in send_addr;
        uv_ip4_addr("127.0.0.1", 2000, &send_addr);

        uv_udp_send(send_req, &send_socket, &udp_broadcast_msg, 1, (const struct sockaddr *)&send_addr, udp_package_sent_cb);

//        char i=0;
//        for(;i<udp_broadcast_msg.len; i++)
//        {
//            printf("%c ",udp_broadcast_msg.base[i]);
//        }
//        printf("\n");

    }
}

void idle_cb(uv_idle_t *handle)
{
    if(++counter1 == 10000000)
    {
        counter1 =0;
//        if(proxy_client != 0)
//        {
//            write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
//            uv_buf_t tcp_msg = buildTcpMessage();
//            uv_write((uv_write_t*) req, (uv_stream_t*)proxy_client, &tcp_msg, 1, echo_write);
//        }
    }
}

void udp_package_received_cb(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
    if (nread <= 0)
    {
        //Read Error or Nothing to read or an Empty UDP package received
        free(buf->base);
        return;
    }

    free(buf->base);
}

void udp_package_sent_cb(uv_udp_send_t *req, int status)
{
    if (status)
    {
        fprintf(stderr, "Send error %s\n", uv_strerror(status));
    }
    else
    {
        //fprintf(stderr, "Send Done\n");
    }
    //free(req->data);
}

