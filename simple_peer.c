//
// Created by Noah Schmalenberger on 10/15/24.
//

#include <unistd.h>     // sleep(3)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#define SIZE 1024

///////////////////////
/* Custom Data Types */
///////////////////////

typedef struct user_node {
    char name[20];
    char ip[20];
    int port;
} user_node;

typedef struct connection {
    user_node* node;
    struct connection* next;
} connection;

typedef struct receiving_info {
    int* server_fd;
    connection** connection_list;
    user_node* host;
} receiving_info;

void sending(connection** connection_list);
void leaving(connection** connection_list, user_node* host);
void *receiving(void *info);
void send_message(connection** connection_list, user_node* host);
void create_network(user_node* host, int* server_fd, connection** connection_list);
void receive_message(int server_fd);

void connect_to_network(user_node host, user_node server);
void receive_client(char* client_info, connection** connection_list, user_node* host);
void receive_connection(char* client_info, connection** connection_list, user_node* host);
void remove_connection(char* client_info, connection** connection_list, user_node* host);

void send_file(connection** connection_list);
void receive_file(int client_socket);

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

    // Create the empty connection list
    connection* connection_list = malloc(sizeof(connection));
    connection_list->node = NULL;
    connection_list->next = NULL;

    // Create the socket descriptor variable for the server and then call the create_network function which will initialize our receiving server.
    int server_fd;
    create_network(&host, &server_fd, &connection_list);

    printf("Connect to NETWORK? (1==yes)");
    int choice;
    scanf("%d", &choice);
    if (choice == 1) {
        // Gather Server Information
        printf("Enter NETWORK ip address: ");
        scanf("%s", server.ip);

        printf("Enter NETWORK port number: ");
        scanf("%d", &server.port);

        // Connects
        connect_to_network(host, server);
    }

    // Call the function to initialize our sending messages loop
    send_message(&connection_list, &host);

    // Server: close socket
    printf("[+]Closing the server connection.\n");
    close(server_fd);

    return 0;
}

void create_network(user_node* host, int* server_fd, connection** connection_list) {
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

    receiving_info* info = malloc(sizeof(receiving_info));
    info->server_fd = server_fd;
    info->connection_list = connection_list;
    info->host = host;

    // Create thread for server to accept connections on
    pthread_t tid;
    pthread_create(&tid, NULL, &receiving, info);
}

void *receiving(void *info)
{

    receiving_info in = *((receiving_info *)info); // Convert the void pointer to a receiving_info type.

    while (1) // Endless loop to receive. Will stop once the user closes the program.
    {
        sleep(2); // Limits this loop to running once every two seconds.

        char strData[255]; // Data to store the received message into
        int client_socket = accept(*in.server_fd, NULL, NULL); // Trying to accept a connection request.

        if (client_socket != -1)
        {
            printf("[+]Receiving...\n");
            recv(client_socket, strData, sizeof(strData), 0);
            printf("[+]Received message\n");

            if (strncmp(strData, "CONNECTION_NEW", 14) == 0)
            {
                receive_client(strData, in.connection_list, in.host);
            }
            if (strncmp(strData, "CONNECTION_ADD", 14) == 0)
            {
                receive_connection(strData, in.connection_list, in.host);
            }

            if (strncmp(strData, "FILE_TRANSFER", 13) == 0)
            {
                receive_file(client_socket);
            }
            if (strncmp(strData, "CONNECTION_RM", 12) == 0)
            {
                remove_connection(strData, in.connection_list, in.host);
            }
            printf("Message: %s\n", strData);
        }
    }
}

void send_message(connection** connection_list, user_node* host)
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
                sending(connection_list);
                break;
            case 2:
                send_file(connection_list);
            break;
            case 0:
                leaving(connection_list, host);
                printf("Leaving\n");
                break;
            default:
                printf("Wrong choice\n");
        }
    } while (choice);
}

// Sending messages to port
void sending(connection** connection_list)
{
    char message[1024] = {0};
    char dummy;
    printf("Enter your message:");
    scanf("%c", &dummy); // Clear the return
    scanf("%[^\n]s", message);

    connection* iterator = *connection_list;
    while (iterator->node != NULL) {

        char buffer[2000] = {0};
        char *ip = iterator->node->ip;
        int port = iterator->node->port;
        int connectStatus;

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

        sprintf(buffer, "%s", message);

        send(client_fd, buffer, sizeof(buffer), 0);

        printf("[+]Message sent.\n");
        printf("[+]Closing the client connection.\n");
        close(client_fd);

        if (iterator->next != NULL) { iterator = iterator->next; }
        else { break; }
    }
}

void leaving(connection** connection_list, user_node* host)
{
    connection* iterator = *connection_list;
    while (iterator->node != NULL) {

        char buffer[2000] = {0};
        char *ip = iterator->node->ip;
        int port = iterator->node->port;
        int connectStatus;

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

        // Create and send connection string
        char connection_string[80] = "CONNECTION_RM";
        strncat(connection_string, host->name, 20);
        strncat(connection_string, ",", 2);
        strncat(connection_string, host->ip, 20);
        strncat(connection_string, ",", 2);
        char send_port[10];
        sprintf(send_port, "%d", host->port);
        strncat(connection_string, send_port, 10);

        printf("connection string: %s\n", connection_string);
        //send(client_fd, buffer, sizeof(buffer), 0);
        send(client_fd, connection_string, sizeof(connection_string), 0);

        printf("[+]Closing the client connection.\n");
        close(client_fd);

        if (iterator->next != NULL) { iterator = iterator->next; }
        else { break; }
    }
}

// Receive new clients and puts them into our connection_list
void receive_client(char* client_info, connection** connection_list, user_node* host)
{
    // Remove the "CONNECTION_INFO" tag from the message
    client_info+=14;

    // Message to send to existing connections
    char connection_string[80] = "CONNECTION_ADD";
    strncat(connection_string, client_info, 60);

    // Setup client connection information
    connection *head = malloc(sizeof(connection));
    head->node = malloc(sizeof(user_node));
    head->next = NULL;

    // Get the name
    char* token = strtok(client_info, ",");
    strcpy(head->node->name, token);

    // Get the ip
    token = strtok(NULL, ",");
    strcpy(head->node->ip, token);

    // Get the port
    token = strtok(NULL, ",");
    head->node->port = atoi(token);

    // To receive the client we need to (1) send everyone in our connection list the client,
    // (2) send the client our connection list and (3) add the client to our connection list

    // (1) Send the client to our connection list
    connection* iterator = *connection_list;
    while (iterator->node != NULL) {

        char buffer[2000] = {0};
        char *ip = iterator->node->ip;
        int port = iterator->node->port;
        int connectStatus;

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

        sprintf(buffer, "%s", connection_string);

        send(client_fd, buffer, sizeof(buffer), 0);

        printf("[+]Message sent.\n");
        printf("[+]Closing the client connection.\n");
        close(client_fd);

        if (iterator->next != NULL) { iterator = iterator->next; }
        else { break; }
    }

    // (2) Send the client our connection list before adding the client
    // First send the client our info
    char buffer[2000] = {0};
    char *ip = head->node->ip;
    int port = head->node->port;
    int connectStatus;

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

    // Create and send connection string
    char connection_string2[80] = "CONNECTION_ADD";
    strncat(connection_string2, host->name, 20);
    strncat(connection_string2, ",", 2);
    strncat(connection_string2, host->ip, 20);
    strncat(connection_string2, ",", 2);
    char send_port[10];
    sprintf(send_port, "%d", host->port);
    strncat(connection_string2, send_port, 10);

    sprintf(buffer, "%s", connection_string2);

    send(client_fd, buffer, sizeof(buffer), 0);

    printf("[+]Message sent.\n");
    printf("[+]Closing the client connection.\n");
    close(client_fd);

    // Then send the client the rest of the list
    iterator = *connection_list;
    while (iterator->node != NULL) {
        char buffer[2000] = {0};
        char *ip = head->node->ip;
        int port = head->node->port;
        int connectStatus;

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

        // Create and send connection string
        char connection_string[80] = "CONNECTION_ADD";
        strncat(connection_string, iterator->node->name, 20);
        strncat(connection_string, ",", 2);
        strncat(connection_string, iterator->node->ip, 20);
        strncat(connection_string, ",", 2);
        char send_port[10];
        sprintf(send_port, "%d", iterator->node->port);
        strncat(connection_string, send_port, 10);

        sprintf(buffer, "%s", connection_string);

        send(client_fd, buffer, sizeof(buffer), 0);

        printf("[+]Message sent.\n");
        printf("[+]Closing the client connection.\n");
        close(client_fd);

        if (iterator->next != NULL) { iterator = iterator->next; }
        else { break; }
    }

    // (3) Add the client to our connection list
    if ((*connection_list)->node == NULL) {
        *connection_list = head;
    } else {
        connection* iterator = *connection_list;
        while (iterator->next != NULL) {
            iterator = iterator->next;
        }
        iterator->next = head;
    }

    printf("new name: %s\n", head->node->name);
}

void receive_connection(char* client_info, connection** connection_list, user_node* host) {
    // Remove the "CONNECTION_INFO" tag from the message
    client_info+=14;

    // Setup client connection information
    connection *head = malloc(sizeof(connection));
    head->node = malloc(sizeof(user_node));
    head->next = NULL;

    // Get the name
    char* token = strtok(client_info, ",");
    strcpy(head->node->name, token);

    // Get the ip
    token = strtok(NULL, ",");
    strcpy(head->node->ip, token);

    // Get the port
    token = strtok(NULL, ",");
    head->node->port = atoi(token);


    // Add the client to our connection list
    if ((*connection_list)->node == NULL) {
        *connection_list = head;
    } else {
        connection* iterator = *connection_list;
        while (iterator->next != NULL) {
            iterator = iterator->next;
        }
        iterator->next = head;
    }

    printf("new name: %s\n", head->node->name);
}

void remove_connection(char* client_info, connection** connection_list, user_node* host)
{
    // Remove the "CONNECTION_RM" tag from the message
    client_info+=13;

    // Get the name
    char* token = strtok(client_info, ",");

    // Remove the client from the list
    connection* iterator = *connection_list;
    if (iterator->next == NULL) {
        connection* prev = malloc(sizeof(connection));
        prev->node = NULL;
        prev->next = NULL;
        (*connection_list) = prev;
    }
    else {
        connection* prev;
        printf("\n%s", token);
        printf("\n%s\n", iterator->node->ip);


        while (strcmp(iterator->node->name, token) != 0) {
            printf("HERE\n");
            prev = iterator;
            iterator = iterator->next;
        }
        // Remove from the list and free the memory
        printf("HERE\n");

        prev->next = iterator->next;
        free(iterator);
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

    // Create and send connection string
    char connection_string[80] = "CONNECTION_NEW";
    strncat(connection_string, host.name, 20);
    strncat(connection_string, ",", 2);
    strncat(connection_string, host.ip, 20);
    strncat(connection_string, ",", 2);
    char send_port[10];
    sprintf(send_port, "%d", host.port);
    strncat(connection_string, send_port, 10);

    printf("connection string: %s\n", connection_string);
    //send(client_fd, buffer, sizeof(buffer), 0);
    send(client_fd, connection_string, sizeof(connection_string), 0);

    printf("[+]Closing the client connection.\n");
    close(client_fd);
}

void send_file(connection** connection_list)
{

    // file stuff and things
    FILE *fp;
    char *filename = "send.txt";
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("[-]Error in reading file");
        exit(1);
    }

    connection* iterator = *connection_list;
    while (iterator->node != NULL) {
        char hello[1024] = {0};

        char *ip = iterator->node->ip;
        int port = iterator->node->port;
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

        if (iterator->next != NULL) { iterator = iterator->next; }
        else { break; }
    }
}

void receive_file(int client_socket) {
    int n;
    FILE *fp;
    char *filename = "recv.txt";
    char buffer[SIZE];

    fp = fopen(filename, "w");
    while (1) {
        n = recv(client_socket, buffer, SIZE, 0);
        if (n <= 0){
            break;
            return;
        }
        fprintf(fp, "%s", buffer);
        bzero(buffer, SIZE); // memset()
    }
    fclose(fp);
}