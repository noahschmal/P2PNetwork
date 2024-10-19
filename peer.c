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
#include <netdb.h>
#define SIZE 1024

//////////////////////
/*       Notes      */
//////////////////////

// Create class diagram

// automatically choose port

// Create start_server()
// Create connect(user) that calls start_server and takes a user and an ip/port to connect to
// Create create(user) that creates an empty network

// Flesh out connection data structure (linked list of connections)

// Make sending() and receiving()
// Sending sends to everyone in the connection list

// Create ping_connections() that checks if all connections are still connected everyone so often

// Messages buffers for sending files, sending messages, and sending streams
// Messages receiving functions for each type (ex: file, message, voice)



///////////////////////
/* Custom Data Types */
///////////////////////

typedef struct user {
    char name[20];
    char SERVER_IP[20];
    int port;
}user;

typedef struct connection {
    struct user* connected_user;
    struct connection* next_connection;
}connection;

void sending();
void receiving(int server_fd);
void *receive_thread(void *server_fd);
void send_message();
void create_network(user* host, int* server_fd);
void send_file();
void receive_file(int server_fd);
void receive_message(int server_fd);

int main(int argc, char const* argv[])
{

    /////////////////////////////
    /* Gather User Information */
    /////////////////////////////

    user host;

    char hostbuffer[256];
    struct hostent *host_entry;
    int hostname;
    struct in_addr **addr_list;

    // Retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));

    // Retrieve IP addresses
    host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL) {
        herror("[-]gethostbyname error");
        exit(1);
    }
    addr_list = (struct in_addr **)host_entry->h_addr_list;

    // Print out the IP Addresses
    printf("IP address: \n");
    printf("[0] Enter custom address\n");
    for (int i = 0; addr_list[i] != NULL; i++) {
        printf("[%d] %s\n", i+1, inet_ntoa(*addr_list[i]));
    }

    // Have the user choose which ip address to use
    //                                                                              Add protection for choice
    int ip_choice;
    printf("Enter your ip selection: ");
    scanf("%d", &ip_choice);
    if (ip_choice != 0) {
        strcpy(host.SERVER_IP, inet_ntoa(*addr_list[ip_choice - 1]));
    } else if (ip_choice == 0) {
        printf("Enter your IP: ");
        scanf("%s", host.SERVER_IP);
    }

    printf("Enter name: ");
    scanf("%s", host.name);

    printf("Enter your port number: ");
    scanf("%d", &host.port);

    int server_fd;
    create_network(&host, &server_fd);

    send_message();

    // Server: close socket
    printf("[+]Closing the server connection.\n");
    close(server_fd);

    return 0;
}

void create_network(user* host, int* server_fd) {
    ///////////////////////////////
    /* Create Server Information */
    ///////////////////////////////
    printf("[-+-]Server ip: %s:%d\n", host->SERVER_IP, host->port);

    // Server: Connection information
    int connectStatus;

    // Server: Socket and address
    struct sockaddr_in server_addr;

    // Server: Create socket
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(*server_fd < 0) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    // Server: Set address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = host->port;
    server_addr.sin_addr.s_addr = inet_addr(host->SERVER_IP); //INADDR_ANY;

    // Server: Bind socket
    connectStatus = bind(*server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(connectStatus < 0) {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successfull.\n");

    // Server: Listen with socket
    if (listen(*server_fd, 10) == 0) {
        printf("[+]Listening....\n");
    } else {
        perror("[-]Error in listening");
        exit(1);
    }

    // Create thread for server to accept connections on
    pthread_t tid;
    pthread_create(&tid, NULL, &receive_thread, server_fd);
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
            case 2:
                send_file();
                break;
            case 0:
                printf("Leaving\n");
                break;
            default:
                printf("Wrong choice\n");
        }
    } while (choice);
}

//Sending messages to port
void sending()
{
    char buffer[2000] = {0};
    char hello[1024] = {0};

    char *ip = "10.108.41.167"; // 68.234.244.147
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
    server_addr.sin_addr.s_addr = inet_addr(ip);//INADDR_ANY;//inet_addr(ip);

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

        if (strcmp(strData, "FILE_TRANSFER") == 0) {
            printf("[+] Receiving File...\n");
            receive_file(client_socket);
        } else {

            printf("Message: %s\n", strData);
        }
    }
}

void receive_message(int server_fd) {

}

void receive_file(int client_fd) {
    int n;
    FILE *fp;
    char *filename = "recv.c";
    char buffer[SIZE];

    fp = fopen(filename, "w");
    while (1) {
        n = recv(client_fd, buffer, SIZE, 0);
        if (n <= 0){
            break;
            return;
        }

        fprintf(fp, "%s", buffer);
        bzero(buffer, SIZE); // memset()
    }
    fclose(fp);
    return;
}

void send_file() {

    // file stuff and things
    FILE *fp;
    char *filename = "peer.c";
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("[-]Error in reading file");
        exit(1);
    }


    char hello[1024] = {0};

    char *ip = "10.189.51.56"; // 68.234.244.147 // Josh: 68.234.244.163
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
    printf("[+]Client socket created successfully\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);//INADDR_ANY;//inet_addr(ip);

    connectStatus = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(connectStatus == -1) {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server\n");

    int n;
    char data[SIZE] = {0};
    char buffer[32] = "FILE_TRANSFER";

    send(client_fd, buffer, sizeof(buffer), 0);

    while(fgets(data, SIZE, fp) != NULL) {
        printf("sending: %s\n", data);
        if (send(client_fd, data, sizeof(data), 0) == -1) {
            perror("[-]Error in sending file\n");
            exit(1);
        }
        bzero(data, SIZE);
    }

    printf("[+]File sent\n");
    printf("[+]Closing the client connection\n");
    close(client_fd);
}