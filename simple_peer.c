//
// Created by Noah Schmalenberger on 10/15/24.
//

//#include "simple_peer.h"
#include <unistd.h>     // sleep(3)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>

///////////////////////
/* Custom Data Types */
///////////////////////

typedef struct user_node {
    char name[20];
    char ip[20];
    int port;
} user_node;

typedef struct connection {
    struct user_node* node;
    struct connection* next;
} connection;

void sending();
void *receiving(void *server_fd);
//void *receive_thread(void *server_fd);
void send_message();
void create_network(user_node* host, int* server_fd);
void send_file();
void receive_file(int server_fd);
void receive_message(int server_fd);

void connect_to_network(user_node host, user_node server);
void receive_client(connection** list, int client_fd);

int main(int argc, char const* argv[])
{

    /////////////////////////////
    /* Gather User Information */
    /////////////////////////////

    // Create the host node for the client. This will be sent to other nodes on the network to attach to their connection_list.
    user_node host;
    user_node server;

    // Have the user input their information that will be used to create their node and also open their server socket.
    printf("Enter your IP: ");
    scanf("%s", host.ip);

    printf("Enter name: ");
    scanf("%s", host.name);

    printf("Enter your port number: ");
    scanf("%d", &host.port);

    // Gather Server Information
    printf("Enter SERVER ip address: ");
    scanf("%s", server.SERVER_IP);

    printf("Enter SERVER port number: ");
    scanf("%d", &server.port);

    // Connects
    connect_to_network(host, server);

    // Create the socket descriptor variable for the server and then call the create_network function which will initialize our receiving server.
    int server_fd;
    create_network(&host, &server_fd);

    // Call the function to initialize our sending messages loop
    send_message();

    // Server: close socket
    printf("[+]Closing the server connection.\n");
    close(server_fd);

    return 0;
}

void create_network(user_node* host, int* server_fd) {
    ///////////////////////////////
    /* Create Server Information */
    ///////////////////////////////
    printf("[-+-]Server ip: %s:%d\n", host->ip, host->port); // Useful for debugging and checking user input. This can be shared with other people looking to connect to the network.

    // Server: Connection information
    int connectStatus;

    // Server: create address variable
    struct sockaddr_in server_addr;

    // Server: Create socket
    // AF_INET specifies IPv4
    // SOCK_STREAM specifies the communication type, in this case it is TCP
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Check to make sure the server socket was created successfully.
    if(*server_fd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    // Server: Set address, this will be used for when we bind our socket to an address using the information we collected from the user in main()
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = host->port;
    server_addr.sin_addr.s_addr = inet_addr(host->ip);

    // Server: Bind socket
    connectStatus = bind(*server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Check to make sure the socket binding was successful
    if(connectStatus < 0) {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successful.\n");

    // Server: Wait for a client to make a connection
    int backlog = 10; // Maximum length of the queue of connection that can be waiting for this socket.
    if (listen(*server_fd, backlog) == 0) {
        printf("[+]Listening....\n");
    } else {
        perror("[-]Error in listening");
        exit(1);
    }

    // Create thread for server to accept connections on
    pthread_t tid;
    pthread_create(&tid, NULL, &receiving, server_fd);
}

void *receiving(void *s_fd)
{

    int server_fd = *((int *)s_fd); // Convert the void pointer to an int.

    while (1) // Endless loop to receive. Will stop once the user closes the program.
    {
        sleep(2); // Limits this loop to running once every two seconds.

        char strData[255]; // Data to store the received message into
        int client_socket = accept(server_fd, NULL, NULL); // Trying to accept a connection request.

        if (client_socket != -1)
        {
            printf("[+]Receiving...\n");
            recv(client_socket, strData, sizeof(strData), 0);

            if (strcmp(strData, "CONNECTION_INFO") == 0)
            {
                receive_client(list, client_socket);
            }
            printf("Message: %s\n", strData);
        }
    }
}

void send_message()
{
    ///////////////////////////////
    /* Create Client Information */
    ///////////////////////////////
    int choice;
    printf("\n[+]At any point in time press the following:\n1. Send message\n2. Send file\n0. Quit\n");
    printf("\nEnter choice: ");
    do
    {
        scanf("%d", &choice);
        switch (choice)
        {
            case 1:
                sending();
                break;
            case 0:
                printf("Leaving\n");
                break;
            default:
                printf("Wrong choice\n");
        }
    } while (choice);
}

// Sending messages to port
void sending()
{
    char buffer[2000] = {0};
    char hello[1024] = {0};

    char *ip = "68.234.244.147"; // 68.234.244.147
    int port;
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
    printf("[+]Client socket created successfully.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

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
    sprintf(buffer, "[PORT:%d] says: %s", port, hello);

    send(client_fd, buffer, sizeof(buffer), 0);

    printf("[+]Message sent.\n");
    printf("[+]Closing the client connection.\n");
    close(client_fd);
}

// Receive new clients and puts them into our connection_list
void receive_client(connection** list, int client_fd)
{
    connection *head;
    head.connected_user = malloc(sizeof(user));
    head.next_connection = NULL;

    char buffer[20];
    recv(client_fd, buffer, sizeof(buffer), 0); // NAME
    strcpy(head->node->name, buffer);
    bzero(buffer, sizeof(buffer)); // memset()

    recv(client_fd, buffer, sizeof(buffer), 0); // IP
    strcpy(head->node->ip, buffer);
    bzero(buffer, sizeof(buffer)); // memset()

    recv(client_fd, buffer, sizeof(buffer), 0); // PORT
    head->node->port = atoi(buffer);

    if (*list->next == NULL) {
        *list = head;
    } else {
        connection* iterator = *list;
        while (iterator->next != NULL) {
            iterator = iterator->next;
        }
        iterator->next = head;
    }
}

void connect_to_network(user_node host, user_node server)
{
    char *ip = server.ip;
    int port = server.port;
    int connectStatus;

    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Client socket created successfully\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    connectStatus = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(connectStatus == -1) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server\n");

    int n;
    char data[1024] = {0};
    char buffer[32] = "CONNECTION_INFO";

    send(client_fd, buffer, sizeof(buffer), 0);

    char send_name[20];
    strcpy(send_name, host.name);

    send(client_fd, send_name, sizeof(send_name), 0);

    char send_ip[20];
    strcpy(send_ip, host.ip);

    send(client_fd, send_ip, sizeof(send_ip), 0);

    char send_port[10];
    sprintf(send_port, "%d", host.port);

    send(client_fd, send_port, sizeof(send_port), 0);

    printf("[+]Closing the client connection.\n");
    close(client_fd);
}