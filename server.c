#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>



/* Global client socket to close in signal handler. */
//FIX ME: this will have to be an array of all client connections later.
int clientsocket;


/* ^c signal handler. */
void sig_handler(int sigNum)
{
	printf("\nClosing client sockets...\n");
    close(clientsocket);
	exit(0);
}


/**
 * Receiving function that takes an arbitrary value as a parameter
 * and casts it to an integer to receive on that file descriptor.
 * 
 * @param *arg a value of any type, recommended integer.
 * @return void * adaptable to return different types.
 */
void *recv_msg(void *arg)
{
    while (1)
    {
        /* Cast the parameter to an int. */
        int clientsocket = *((int *)arg);
        char line[5000];
        recv(clientsocket, line, 5000, 0);
        printf("\n> From Client: %s\n", line);
    }

    return arg;
}


/**
 *
 */
int main(int argc, char **argv)
{
    signal(SIGINT, sig_handler);

    int port, status;
    char cp_num[16];
    pthread_t recv;

    printf("Enter a port number: ");
    fgets(cp_num, 16, stdin);
    port = atoi(cp_num);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9876);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(sockfd, 10);

    socklen_t len = sizeof(clientaddr);
    clientsocket = accept(sockfd, (struct sockaddr *)&clientaddr, &len);

    if ((status = pthread_create(&recv, NULL, recv_msg, &clientsocket)) != 0)
    {
        fprintf(stderr, "Thread create error %d: %s\n", status, strerror(status));
        exit(1);
    }

    while (1)
    {
        printf("Enter a line: ");
        char line[5000];
        fgets(line, 5000, stdin);
        send(clientsocket, line, strlen(line) + 1, 0);
    }

    return 0;
}
