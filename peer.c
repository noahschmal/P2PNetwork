//
// Created by Noah Schmalenberger on 10/15/24.
//

//#include "peer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char const* argv[])
{
    char *ip = "127.0.0.1";
    int port = 8080;
    int connectStatus;

    int sockfd;
    struct sockaddr_in server_addr;

    char name[20];
    printf("Enter name:");
    scanf("%s", name);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = INADDR_ANY;//inet_addr(ip);

    connectStatus = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(connectStatus == -1) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server.\n");

    char serMsg[255] = "Message from the client to the server \'Hello test!\' ";
    send(sockfd, serMsg, sizeof(serMsg), 0);

    return 0;
}