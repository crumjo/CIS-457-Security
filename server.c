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

#define STDIN 0

/* Global client socket to close in signal handler. */
//FIX ME: this will have to be an array of all client connections later.
int clientsocket;

/* ^c signal handler. */
void sig_handler(int sigNum)
{
    printf("\nClosing client fds...\n");
    close(clientsocket);
    exit(0);
}

void *connection_handler(void *arg)
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

    char cp_num[16];
    char welcome[] = "Welcome to the chat server!\n";
    int port, sockfd, new_socket;
    int max_clients = 10;
    int sockets[max_clients];
    fd_set fds;

    printf("Enter a port number: ");
    fgets(cp_num, 16, stdin);
    port = atoi(cp_num);

    for (int i = 0; i < max_clients; i++)
    {
        sockets[i] = 0;
    }

    /* Create the socket. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed.\n");
        return 1;
    }

    /* Socket options for multiple connections. */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("Set socket options failed.\n");
        return 1;
    }

    struct sockaddr_in serveraddress, clientaddr;
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(port);
    serveraddress.sin_addr.s_addr = INADDR_ANY;

    /* Bind to user entered port. */
    bind(sockfd, (struct sockaddr *)&serveraddress, sizeof(serveraddress));

    /* Listen for max pending connections. */
    listen(sockfd, max_clients);

    socklen_t len = sizeof(serveraddress);

    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);
    FD_SET(STDIN, &fds);

    while (1)
    {
        fd_set tmp_set = fds;
        len = sizeof(clientaddr);

        select(FD_SETSIZE, &tmp_set, NULL, NULL, NULL);

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            /* Check for connections. */
            if (FD_ISSET(i, &tmp_set))
            {
                if (i == sockfd)
                {
                    if ((new_socket = accept(sockfd, (struct sockaddr *)&clientaddr, &len)) < 0)
                    {
                        perror("New client accept failed.\n");
                        return 1;
                    }

                    FD_SET(new_socket, &fds);
                    for (int j = 0; j < max_clients; j++)
                    {
                        if (sockets[j] == 0)
                        {
                            sockets[j] = new_socket;
                            break;
                        }
                    }

                    printf("New connection, sock fd:%d \t ip: %s \t port: %d\n",
                           new_socket, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

                    if ((send(new_socket, welcome, strlen(welcome), 0)) != strlen(welcome))
                    {
                        perror("Send failed.\n");
                        return 1;
                    }
                }
                else if (i == STDIN)
                {
                    char input[1024];
                    fgets(input, 1024, stdin);
                    printf("Stdin :%s:\n", input);
                    for (int k = 0; k < max_clients; k++)
                    {
                        if (sockets[k] == 0)
                            continue;
                        send(sockets[k], input, strlen(input) + 1, 0);
                    }
                }
                else
                {
                    char line[5000];
                    recv(i, line, 5000, 0);
                    printf("Got from client: %s\n", line);
                    send(i, line, strlen(line) + 1, 0);
                    // close(i);
                    // FD_CLR(i, &fds);
                }
            }
        }
    }
    return 0;
}
