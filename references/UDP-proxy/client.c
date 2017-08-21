#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEST_IP "10.60.60.11"
#define DEST_PORT 7747

int main(int argc, char* argv[])
{
	char arg[1000];
	const char* server_name = DEST_IP;
	const int server_port = DEST_PORT;

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	// creates binary representation of server name
	// and stores it as sin_addr
	// http://beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html
	inet_pton(AF_INET, server_name, &server_address.sin_addr);

	// htons: port in network order format
	server_address.sin_port = htons(server_port);

	// open socket
	int sock;
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("could not create socket\n");
		return 1;
	}

	unsigned int count = 0;
	while(1)
	{
		// data that will be sent to the server
		char data_to_send[1000];
		//itoa(count++, data_to_send, 10);
		sprintf(data_to_send,"%d",count++);
		//strcpy(&data_to_send, arg);
		// send data
		int len =
			sendto(sock, data_to_send, strlen(data_to_send), 0,
				   (struct sockaddr*)&server_address, sizeof(server_address));

		// received echoed data back
		char buffer[100];
		recvfrom(sock, buffer, len, 0, NULL, NULL);

		buffer[len] = '\0';
		printf("recieved: '%s'\n", buffer);
		//usleep(1);
	}
	// close the socket
	close(sock);
	return 0;
}
