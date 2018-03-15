#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>


void *recv_msg(void *arg)
{
    while (1)
    {
        int clientsocket = *((int *)arg);
        char line[5000];
        recv(clientsocket, line, 5000, 0);
        printf("\n> From Client: %s\n", line);
    }

    return arg;
}

int main(int argc, char **argv)
{
    int port, status;
    char cp_num[16];
    pthread_t recv;

    printf("Enter a port number: ");
    fgets(cp_num, 16, stdin);
    port = atoi(cp_num);
    printf("Port number :%d:\n", port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9876);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(sockfd, 10);

    socklen_t len = sizeof(clientaddr);
    int clientsocket = accept(sockfd, (struct sockaddr *)&clientaddr, &len);

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

    close(clientsocket);
}
