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
	char *command = (char*) malloc(sizeof(char)*20);
	char *text = (char*) malloc(sizeof(char)*100);
	int admin = 0;

	while(strcmp(receive, "/quit\n") != 0)
	{
		recv(targs.clientsocket, receive, 5000, 0);
		strcpy(text, receive); // copy receive in
		if (strchr(text, ' ')  != NULL)
		{
			strcpy(command, strsep(&text, " ")); //get command
		}
		if (strchr(text, '\n') != NULL)
		{
			strcpy(text, strsep(&text, "\n"));
		}
		

		if (strcmp(command, "/admin") == 0)
		{
			char msg[40];
			if (atoi(text) == adminpassword)
			{
				printf("Admin rights granted to %s\n", targs.username);
				strcpy(msg, "admin rights granted");
				admin = 1;
				send (targs.clientsocket, msg, sizeof(msg), 0);
			}
			else
			{
				printf("Admin rights not granted to %s\n", targs.username);
				strcpy(msg, "admin rights denied");
				admin = 0;
				send (targs.clientsocket, msg, sizeof(msg), 0);
			}
		}
		else if (strcmp(command, "/w") == 0)
		{
			char *dstusername = (char*) malloc(sizeof(char)*100);
			strcpy (dstusername, strsep(&text, " "));
			for (int i = 0; i < 1024; i++)
			{
				if (strcmp(client_list[i], dstusername) == 0)
				{
					send(i, text, sizeof(text), 0);
				}
			}
			free(dstusername);
		}
		else if (strcmp(command, "/broadcast") == 0)
		{
			for (int i = 0; i < 1024; i++)
			{
				if (strcmp(client_list[i], "/NULL") != 0)
				{
					send(i, text, sizeof(text), 0);
				}
			}
		}
		else if (strcmp(command, "/kick") == 0)
		{
			char msg[100];

			if (admin)
			{
				for (int i = 0; i < 1024; i++)
				{
					if (strcmp(client_list[i], text) == 0)
					{
						strcpy(msg, "/quit\n");
						strcpy(receive, "/quit\n");
						send(i, msg, sizeof(msg), 0);
					}
				}
			}
		}
		else if (strcmp(command, "/clientlist") == 0)
		{
			char msg [1024*100];
			for (int i = 0; i < 1024; i++)
			{
				if (strcmp(client_list[i], "/NULL") != 0)
				{
					strcat (msg, client_list[i]);
					strcat (msg, "\n");
				}
			}
			
			send (targs.clientsocket, msg, sizeof(msg), 0);
		}
		//strsep on received string
		//First arg is command (/admin, /w, /broadcast, /client list, /kick 'username')
		//Second arg is text (admin -> password, w -> send text, broadcast -> send text, client -> list)
		//
		//send from each command received
		//admin -> "success" or "failed"
		//username -> "success" or "error"
		//broadcast -> "success" or "No other clients"
		//client list -> one string of all clients (separated by \n)
		//Command list ->
		//kick -> send /quit message to designated username


	}
	printf("Client %s closed\n", targs.username);
	free(command);
	free(text);
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

	char *username = (char*)malloc(sizeof(char)*100);
	int clientsocket;

	for (int i = 0; i < 1024; i++)
	{
		strcpy(client_list[i], "/NULL");
	}

	while(1)
	{
		socklen_t len = sizeof(clientaddr);
		clientsocket = accept(sockfd,(struct sockaddr*)&clientaddr,&len);

		//Receive client username
		
			if (recvfrom(clientsocket, username, 100, 0, (struct sockaddr*)&clientaddr,&len) != -1 )
			{
				strcpy(username, strsep(&username, "\n"));
				strcpy (client_list[clientsocket], username);
				printf("New Client: %s\n", username);
			}	
			else
				printf("recv error");
		

		struct thread_args *args = malloc(sizeof(struct thread_args));
		memcpy(&args->clientsocket, &clientsocket, sizeof(int));
		strcpy(args->username, username);

		pthread_t tid;
		pthread_create(&tid, NULL, worker, args);
		pthread_detach(tid);
	}

	free(username);
}


