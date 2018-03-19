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
	int clientsocket;
	char username[100];
};

char client_list[1024][100];
int adminpassword = 1234;


void* worker(void* args)
{

	struct thread_args targs;
	memcpy(&targs, args, sizeof(struct thread_args));
	char receive[5000];


	while(strcmp(receive, "quit*\n") != 0)
	{
		recv(targs.clientsocket, receive, 5000, 0);
		printf("\nReceived from client: %s\n", receive);


		//strsep on received string
		//First arg is command (/admin, /'username', /broadcast, /client list, /kick 'username')
		//Second arg is text (admin -> password, 'username' -> send text, broadcast -> send text, client -> list)
		//
		//send from each command received
		//admin -> "success" or "failed"
		//username -> "success" or "error"
		//broadcast -> "success" or "No other clients"
		//client list -> one string of all clients (separated by \n)
		//kick -> send /quit message to designated username


	}
	return 0;
}

int main(int argc, char **argv)
{

	int sockfd = socket(AF_INET,SOCK_STREAM,0);

	int port;

	printf("Welcome to a simple server\n\n");
	printf("Please provde a port number: ");

	scanf("%d", &port);

	struct sockaddr_in serveraddr,clientaddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(port);
	serveraddr.sin_addr.s_addr=INADDR_ANY;

	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	listen(sockfd,10);

	char username[100];
	int clientsocket;

	for (int i = 0; i < 1024; i++)
	{
		strcpy(client_list[i], "NULL");
	}

	while(1)
	{
		socklen_t len = sizeof(clientaddr);
		clientsocket = accept(sockfd,(struct sockaddr*)&clientaddr,&len);


		//Receive client username
		if (recvfrom(clientsocket, username, 100, 0, (struct sockaddr*)&clientaddr,&len) != -1 )
		{
			strcpy (client_list[clientsocket], username);
			printf("New Client: %s", username);
		}	
		else
			printf("recv error");

		struct thread_args *args = malloc(sizeof(struct thread_args));
		memcpy(&args->clientsocket, &clientsocket, sizeof(int));
		memcpy(&args->username, &username, sizeof(username));

		pthread_t tid;
		pthread_create(&tid, NULL, worker, args);
		pthread_detach(tid);
	}
}


