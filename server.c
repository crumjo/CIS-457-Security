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
    char welcome[] = "Welcome to the chat server! You are user ";
    int port, sockfd, new_socket;
    int max_clients = 10;
    int wlen = strlen(welcome);
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

                    printf("New connection:    sock fd: %d \t ip: %s \t  port: %d\n",
                           new_socket, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

                    char sock[3];
                    snprintf(sock, sizeof(sock), "%d", new_socket);

                    memcpy(welcome + wlen, sock, sizeof(sock));

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
                    char line[5000], line2[5000];
                    char type[5];
                    recv(i, line, 5000, 0);
                    memcpy(type, line, 4);
                    //printf("Type: %s\n", type);

                    /* Broadcast message to all users. */
                    if (strcmp(type, "cast") == 0)
                    {
                        memcpy(line2, line + 4, strlen(line) - 4);
                        printf("Broadcast message to all users.\n");
                        for (int l = 0; l < max_clients; l++)
                        {
                            if (sockets[l] != 0 || sockets[l] != i)
                            {
                                send(l, line2, strlen(line2) + 1, 0);
                            }
                        }
                    }

                    /* Send list to all users. */
                    else if (strcmp(type, "list") == 0)
                    {
                        printf("List requested...\n");
                        char *list = (char *)malloc(512 * sizeof(char));
                        strcat(list, "All users on the server:\n");
                        for (int m = 0; m < max_clients; m++)
                        {
                            if (sockets[m] != 0)
                            {
                                //printf("Socket: %d", sockets[m]);
                                char element[3];
                                snprintf(element, sizeof(element), "%d", sockets[m]);
                                // printf("Element: %s\n", element);
                                // printf("Sockets: %d\n", sockets[m]);
                                // printf("Length: %lu\n", strlen(list));
                                strcat(list, element);
                                strcat(list, "\n");
                            }
                        }
                        // printf("List :%s:\n", list);
                        send(i, list, strlen(list) + 1, 0);
                        free(list);
                    }

                    /* Send to a specific user. */
                    else if (strcmp(type, "user") == 0)
                    {
                        printf("Message for user: %s\n", line);
                        char user[3], msg[5000];

                        memcpy(user, line + 4, 1);
                        memcpy(msg, line + 6, strlen(line) - 7);

                        int iuser = atoi(user);
                        printf("Request to send to user %d.\n", iuser);
                        int found = 0;
                        for (int n = 0; n < max_clients; n++)
                        {
                            if (sockets[n] == iuser)
                            {
                                send(iuser, msg, strlen(msg) + 1, 0);
                                printf("Message sent to user %d.\n", n);
                                found = 1;
                                break;
                            }
                        }

                        if (found == 0)
                        {
                            send(i, "The user could not be found.\n", 23, 0);
                        }
                    }

                    /* Admin. */
                    else if(strcmp(type, "kick") == 0)
                    {
                        char password[5], user[3];
                        
                        memcpy(user, line + 4, 1);
                        memcpy(password, line + 6, 4);
                        
                        int iuser = atoi(user);

                        /* Verify password. */
                        if (strcmp(password, "1234") == 0)
                        {
                            printf("Admin permission verified.\n");
                            send(iuser, "", 1, 0);
                            close(iuser);
                        }
                    }

                    /* Somebody disconnected. */
                    else if (strlen(line) == 0)
                    {
                        printf("%d has disconnected.\n", i);
                        close(i);
                        for (int o = 0; o < max_clients; o++)
                        {
                            if (sockets[o] == i)
                            {
                                sockets[o] = 0;
                                // printf("Removed client %d\n", i);
                                break;
                            }
                        }
                    }

                    else
                    {
                        printf("> From client: %s\n", line);
                        // send(i, line, strlen(line) + 1, 0);
                    }
                    // close(i);
                }
            }
        }
    }
    return 0;
}
