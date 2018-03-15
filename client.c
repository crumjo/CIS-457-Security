#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>



/* Global server socket to close in signal handler. */
int sockfd;


/* ^c signal handler. */
void sig_handler(int sigNum)
{
	printf("\nClosing server socket...\n");
    close(sockfd);
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
        int sockfd = *((int *)arg);
        char line[5000];
        recv(sockfd, line, 5000, 0);
        printf("\n> From Server: %s\n", line);
    }

    return arg;
}


/**
 * 
 */
int main(int argc, char **argv)
{
    signal(SIGINT, sig_handler);

    char cport[5];
    char ip[16];
    int status;
    pthread_t recv;

    printf("Enter an IP address: ");
    fgets(ip, 16, stdin);

    /* Remove trailing newline. */
    int len = (int)strlen(ip);
    if (ip[len - 1] == '\n')
    {
        ip[len - 1] = '\0';
    }

    printf("Enter a port number: ");
    fgets(cport, 16, stdin);
    int port = atoi(cport);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("There was an error creating the socket\n");
        return 1;
    }

    printf("Connected to port %d\n", port);

    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9876);
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int e = connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (e < 0)
    {
        printf("There was an error connecting\n");
        return 1;
    }

    if ((status = pthread_create(&recv, NULL, recv_msg, &sockfd)) != 0)
    {
        fprintf(stderr, "Thread create error %d: %s\n", status, strerror(status));
        exit(1);
    }

    while (1)
    {
        printf("Enter a line: ");
        char line[5000];
        fgets(line, 5000, stdin);
        send(sockfd, line, strlen(line) + 1, 0);
    }

    return 0;
}
