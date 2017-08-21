#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>
# include "/usr/include/libsocket/libinetsocket.h"

#define MAX_THREADS 32767
#define PACK_SIZE 270			// maximum size of a packet string
#define SERVER_PORT 7747
#define IP_SIZE 127
#define PORT_SIZE 6

#define LOCAL_IP "10.60.60.11"
#define LOCAL_PORT "7747"
#define DEST_IP "10.60.60.11"
#define DEST_PORT "8877"

typedef struct Request{
	char ip[IP_SIZE];
	char port[PORT_SIZE];
	char buf[PACK_SIZE];
} Request;

/* Socket Variables - required to be used by all threads */
int sock, length, n, sfd;
struct sockaddr_in server;
//int thread_no;


int sfd2, ret;



void* handle_request(void*);

int main(int argc, char **argv)
{
	int ret;
    sfd = create_inet_server_socket(LOCAL_IP, LOCAL_PORT, LIBSOCKET_UDP, LIBSOCKET_IPv4, 0);

    if (sfd == -1)
    {
	perror("couldn't create server\n");
	exit(1);
    }

    printf("Socket up and running\n");


    sfd2 = create_inet_dgram_socket(LIBSOCKET_IPv4,0);
    if (sfd2 == -1)
    {
    	perror("\ncouldn't create server\n");
    	exit(1);
    }

	////////////////

	/* thread variables */
	//pthread_t threads[MAX_THREADS];
    pthread_t thread_no;
	int rc = 0;

	/* continuously listen on the specified port */
	while(1)
	{
		struct Request *request = (Request*)malloc(sizeof(Request));

		ret = recvfrom_inet_dgram_socket(sfd, request->buf, PACK_SIZE, request->ip, IP_SIZE, request->port, PORT_SIZE, 0, LIBSOCKET_NUMERIC);
		if ( ret < 0 )
		{
			perror(0);
			exit(1);
		}
		printf("\nMainthread received: [%s][%d]\n", request->buf, strlen(request->buf) );

		/* invoke a new thread and pass the recieved request to it */
		rc = pthread_create(&thread_no, NULL, handle_request, (void*)request);
		if(rc)
		{
			printf("A request could not be processed\n");
		}
		else
		{
			//thread_no++;
			pthread_join( thread_no, NULL);
		}
	}
	return 0;
}
void* handle_request(void *req)
{
	printf("\n---handle_request---\n");
	Request *request = (Request*)req;

	//create new socket

    printf("\nSocket2 up and running\n");

    ret = sendto_inet_dgram_socket(sfd2, request->buf, strlen(request->buf), DEST_IP, DEST_PORT, 0);
	if(ret < 0) printf("Send failed");

	strcpy(request->buf, "");
	ret = recvfrom_inet_dgram_socket(sfd2, request->buf, PACK_SIZE, 0, 0, 0, 0, 0, LIBSOCKET_NUMERIC);
	if(ret < 0) printf("Receive failed");

	//Send back to client
	ret = sendto_inet_dgram_socket(sfd2, request->buf, strlen(request->buf), request->ip, request->port, 0);
	if ( ret < 0 ) printf("Send failed");
	printf("\nSent back to client %s port %s: %s (%i)\n",request->ip, request->port, request->buf, ret);

	printf("\nREQUEST COMPLETED\n");
	printf("\nEXIT THREAD\n");
	//sleep(1);
	pthread_exit(NULL);
}
