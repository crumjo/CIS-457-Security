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
	struct sockaddr_in *clientaddr;
};

char client_list[1024][100];
int adminpassword = 1234;


void* worker(void* args)
{

	struct thread_args targs;
	memcpy(&targs, args, sizeof(struct thread_args));
	char receive[5000];
	char *command = (char*) malloc(sizeof(char)*100);
	char *text = (char*) malloc(sizeof(char)*100);
	int admin = 0;

	while(strcmp(receive, "/quit\n") != 0)
	{
		strcpy (command, " ");
		strcpy (text, " ");
		if (recv(targs.clientsocket, receive, 5000, 0) < 1) break;
		strcpy(text, receive); // copy receive in
		if (strchr(text, ' ')  != NULL)
		{
			strcpy(command, strsep(&text, " ")); //get command
		}
		else if (strchr (text, '\n') != NULL)
		{
			strcpy(command, strsep(&text, "\n"));
		}
		if (strchr(text, '\n') != NULL)
		{
			strcpy(text, strsep(&text, "\n"));
		}

		printf("%s : %s\n\n", command, text);
		

		if (strcmp(command, "/admin") == 0)
		{
			char msg[40];
			if (atoi(text) == adminpassword)
			{
				printf("Admin rights granted to %s\n", targs.username);
				strcpy(msg, "admin rights granted\n");
				admin = 1;
				send (targs.clientsocket, msg, sizeof(msg), 0);
			}
			else
			{
				printf("Admin rights not granted to %s\n", targs.username);
				strcpy(msg, "admin rights denied\n");
				admin = 0;
				send (targs.clientsocket, msg, sizeof(msg), 0);
			}
		}
		else if (strcmp(command, "/w") == 0)
		{
			int flag = 0;
			char *dstusername = (char*) malloc(sizeof(char)*100);
			strcpy (dstusername, strsep(&text, " "));
			for (int i = 0; i < 1024; i++)
			{
				if (strcmp(client_list[i], dstusername) == 0)
				{
					flag = 1;
					send(i, text, sizeof(text), 0);
				}
			}

			if (!flag)
			{
				send(targs.clientsocket, "no such client", sizeof("no such client"), 0);
			}
			free(dstusername);
		}
		else if (strcmp(command, "/broadcast") == 0)
		{
			for (int i = 0; i < 1024; i++)
			{
				if (strcmp(client_list[i], "") != 0 && i != targs.clientsocket)
				{
					send(i, text, sizeof(text), 0);
				}
			}
		}
		else if (strcmp(command, "/kick") == 0)
		{
			char msg[1000];

			if (admin)
			{
				for (int i = 0; i < 1024; i++)
				{
					if (strcmp(client_list[i], text) == 0)
					{
						printf("Kicking %s\n", client_list[i]);
						strcpy(msg, "kicked\n");
						strcpy(client_list[i], "");
						send(i, msg, sizeof(msg), 0);
					}
				}
			}
			strcpy(msg, text);
			strcat(msg, " was kicked\n");
			send (targs.clientsocket, msg, sizeof(msg), 0);
		}
		else if (strcmp(command, "/clientlist") == 0)
		{
			printf("Compiling client list\n");
			char msg [1024*100];
			for (int i = 0; i < 1024; i++)
			{
				if (strcmp(client_list[i], "") != 0 && strlen(client_list[i])>1)
				{
					strcat (msg, client_list[i]);
					strcat (msg, "\n");
				}
			}
			printf("Compiled List:\n%s", msg);
			sendto (targs.clientsocket, msg, sizeof(msg), 0,(struct sockaddr*)&targs.clientaddr, sizeof(targs.clientaddr));
		}
	}
	free(command);
	printf("Client %s closed\n", targs.username);
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

	int clientsocket;
	char *username = (char*)malloc(sizeof(char)*100);


	for (int i = 0; i < 1024; i++)
	{
		strcpy(client_list[i], "");
	}

	while(1)
	{

		socklen_t len = sizeof(clientaddr);
		clientsocket = accept(sockfd,(struct sockaddr*)&clientaddr,&len);

		//Receive client username
		int uniqname = 1;
		do
		{
			uniqname = 1;
			if (recvfrom(clientsocket, username, 100, 0, (struct sockaddr*)&clientaddr,&len) != -1 )
			{
				strcpy(username, strsep(&username, "\n"));
				for (int i = 0; i<1024; i++)
				{
					if (strcmp(client_list[i], username) == 0)
					{
						uniqname = 0;
						send(i, &uniqname, sizeof(uniqname), 0);
						break;
					}
				}
			}	
			else
				printf("recv error");

			if (uniqname == 1)
			{
				send(clientsocket, &uniqname, sizeof(uniqname), 0);
				strcpy (client_list[clientsocket], username);
				printf("New Client: %s\n", username);
			}
		} while (uniqname == 0);
		

		struct thread_args *args = malloc(sizeof(struct thread_args));
		memcpy(&args->clientsocket, &clientsocket, sizeof(int));
		memcpy(&args->clientaddr, &clientaddr, sizeof(struct sockaddr_in));
		strcpy(args->username, username);

		pthread_t tid;
		pthread_create(&tid, NULL, worker, args);
		pthread_detach(tid);
	}

	
}


