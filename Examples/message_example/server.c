//
// Created by Noah Schmalenberger on 10/11/24.
//

#include <unistd.h>     // sleep(3)
#include <netinet/in.h> //structure for storing address information
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //for socket APIs
#include <sys/types.h>

int main(int argc, char const* argv[])
{

    // create server socket similar to what was done in
    // client program
    int servSockD = socket(AF_INET, SOCK_STREAM, 0);

    // string store data to send to client
    char serMsg[255] = "Message from the server to the "
                       "client \'Hello Client\' ";

    // define server address
    struct sockaddr_in servAddr;

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9001);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    // bind socket to the specified IP and port
    bind(servSockD, (struct sockaddr*)&servAddr,
         sizeof(servAddr));

    // listen for connections
    listen(servSockD, 1);

    // integer to hold client socket.
    // int clientSocket = accept(servSockD, NULL, NULL);

    // send's messages to client socket
    // send(clientSocket, serMsg, sizeof(serMsg), 0);

    char strData[255];
    while (1)
    {
        int clientSocket = accept(servSockD, NULL, NULL);

        printf("Receiving...\n");
        recv(clientSocket, strData, sizeof(strData), 0);recv(clientSocket, strData, sizeof(strData), 0);

        printf("Message: %s\n", strData);
        strData[0] = '\0';

        sleep(2);
    }

    return 0;
}
