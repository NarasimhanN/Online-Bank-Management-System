#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h> //socket functions
#include <netinet/ip.h> // for sockaddr_in stucture
#include <string.h>
#include <stdlib.h>

// includes
#include "../include/constants.h"
#include "../include/admin.h"
#include "../include/customer.h"

int clients[1000];
int noClients = 0;

// Handles the communication with the client
void connection_handler(int connfd)
{
    printf("Client-%d has connected to the server!\n", noClients);

    char readBuffer[1000], writeBuffer[1000], choice;
    ssize_t readBytes, writeBytes;
    int userChoice;

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connfd, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
        perror("\nERROR_LOG : Error while reading from client");
    else if (readBytes == 0)
        printf("\n\n\tNo data was sent by the client\n");
    else
    {
        userChoice = atoi(readBuffer);

        switch (userChoice)
        {
        case 1:
            // Admin

            admin_operation_handler(connfd);
            break;
        case 2:
            // Customer

            customer_operation_handler(connfd);
            break;
        default:
            exit(EXIT_FAILURE);
        }
    }

    printf("\n\tSERVER LOG : Terminating connection to client-%d!\n", noClients);
}

void main()
{
    int socketFileDescriptor, socketBindStatus, socketListenStatus, connfd;
    struct sockaddr_in serverAddress, clientAddress;

    socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFileDescriptor == -1)
    {
        perror("Error while creating server socket!");
        exit(0);
    }

    serverAddress.sin_family = AF_INET;                // IPv4
    serverAddress.sin_port = htons(PORT);              // Server will listen to port 8080
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Binds the socket to all interfaces

    socketBindStatus = bind(socketFileDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (socketBindStatus == -1)
    {
        perror("Error while binding to server socket!");
        exit(0);
    }
    printf("Server Listener on port %d \n", PORT);
    socketListenStatus = listen(socketFileDescriptor, 10);
    if (socketListenStatus == -1)
    {
        perror("Error while listening for connections on the server socket!");
        close(socketFileDescriptor);
        exit(0);
    }
    noClients = 1;
    puts("Waiting for connections ...");
    int addrlen;
    while (1)
    {
        addrlen = (int)sizeof(clientAddress);
        connfd = accept(socketFileDescriptor, (struct sockaddr *)&clientAddress, &addrlen);
        if (connfd == -1)
        {
            perror("Error while connecting to client!");
            close(socketFileDescriptor);
            exit(EXIT_FAILURE);
        }

        printf("New connection , socket fd is %d , ip is : %s , port : %d\n", connfd, inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        if (!fork())
        {
            // Child will enter this branch
            clients[noClients] = connfd;
            connection_handler(connfd);
            close(connfd);
            exit(0);
        }
        else
        {
            noClients++;
        }
    }

    close(socketFileDescriptor);
}
