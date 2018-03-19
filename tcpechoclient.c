#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>


struct thread_args
{
	int socket;
};

void* worker(void* args)
{
	struct thread_args targs;
	memcpy(&targs, args, sizeof(struct thread_args));
	char receive[5000];

	while(strcmp(receive, "quit*\n") != 0)
	{
		recv(targs.socket, receive, 5000, 0);
		printf("\nReceived from server: %s\n", receive);
	}
	
	return 0;
}

int main(int argc, char** argv)
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	
	if(sockfd<0)
	{
		printf("There was an error creating the socket\n");
		return 1;
	}
	int port;
	char* address = (char*) malloc(sizeof(char)*50);

	printf("Welcome to a simple chat client\n\n");
	printf("Please enter a port: ");

	scanf("%d", &port);

	printf("Please enter an address: ");

	scanf("%s", address);

	struct sockaddr_in serveraddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(port);
	serveraddr.sin_addr.s_addr=inet_addr(address);
	socklen_t len = sizeof(serveraddr);

	int serversocket = connect(sockfd,(struct sockaddr*)&serveraddr,len);
	if(serversocket<0)
	{
		printf("There was an error connecting\n");
		return 1;
	}

	

	char temp[10];
	fgets(temp, 10, stdin);

	char send[5000];
	char username [100];

	printf("Enter a username");
	fgets(username, 100, stdin);
	sendto(sockfd, username, 100, 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

	struct thread_args *args = malloc(sizeof(struct thread_args));
	memcpy(&args->socket, &sockfd, sizeof(int));

	pthread_t tid;
	pthread_create(&tid, NULL, worker, args);
	pthread_detach(tid);
	
	while (strcmp(send, "/quit\n") != 0)
	{
		printf("\nEnter a string: ");
		fgets(send, 5000, stdin);
		sendto(sockfd, send, 5000, 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
		
		//
	}
	
	close(sockfd);
	return 0;
}
