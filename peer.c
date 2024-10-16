//
// Created by Noah Schmalenberger on 10/15/24.
//

//#include "peer.h"
#include <unistd.h>     // sleep(3)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

//                                                                              Create Struct for User

char name[20];
int PORT;

void sending();
void receiving(int server_fd);
void *receive_thread(void *server_fd);

int main(int argc, char const* argv[])
{
    printf("Enter name:");
    scanf("%s", name);

    printf("Enter your port number:");
    scanf("%d", &PORT);

    ///////////////////////////////
    /* Create Server Information */
    ///////////////////////////////

    // Server: Connection information
    char *ip = "127.0.0.1";
    int port = PORT;
    int connectStatus;

    // Server: socket and address
    int server_fd;
    struct sockaddr_in server_addr;

    // Server: Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    // Server: Set address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip); //INADDR_ANY;//inet_addr(ip);

    // Server: Bind socket
    connectStatus = bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(connectStatus < 0) {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successfull.\n");

    // Server: Listen with socket
    if(listen(server_fd, 10) == 0){
        printf("[+]Listening....\n");
    }else{
        perror("[-]Error in listening");
        exit(1);
    }

    // Create thread for server to accept connections on
    pthread_t tid;
    pthread_create(&tid, NULL, &receive_thread, &server_fd);

    ///////////////////////////////
    /* Create Client Information */
    ///////////////////////////////
    int choice;
    printf("\n[+]At any point in time press the following:*****\n1.Send message\n0.Quit\n");
    printf("\nEnter choice:");
    do
    {

        scanf("%d", &choice);
        switch (choice)
        {
            case 1:
                sending();
                break;
            case 0:
                printf("\nLeaving\n");
                break;
            default:
                printf("\nWrong choice\n");
        }
    } while (choice);

    // Server: close socket
    printf("[+]Closing the server connection.\n");
    close(server_fd);

    return 0;
}

//Sending messages to port
void sending()
{
    char buffer[2000] = {0};
    char hello[1024] = {0};

    char *ip = "127.0.0.1";
    int port = 8080;
    int connectStatus;

    printf("Enter the port to send message:"); //Considering each peer will enter different port
    scanf("%d", &port);

    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip); //INADDR_ANY;//inet_addr(ip);

    connectStatus = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(connectStatus == -1) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server.\n");

    char dummy;
    printf("Enter your message:");
    scanf("%c", &dummy); //The buffer is our enemy
    scanf("%[^\n]s", hello);
    sprintf(buffer, "%s[PORT:%d] says: %s", name, PORT, hello);

    send(client_fd, buffer, sizeof(buffer), 0);

    printf("[+]Message sent.\n");
    printf("[+]Closing the client connection.\n");
    close(client_fd);
}

// Calling receiving every 2 seconds
void *receive_thread(void *server_fd)
{
    int s_fd = *((int *)server_fd);
    while (1)
    {
        sleep(2);
        receiving(s_fd);
    }
}

//Receiving messages on our port
void receiving(int server_fd)
{
    char strData[255];
    int client_socket = accept(server_fd, NULL, NULL);

    if (client_socket != -1) {
    printf("Receiving...\n");
    recv(client_socket, strData, sizeof(strData), 0);

    printf("Message: %s\n", strData);
    }
}