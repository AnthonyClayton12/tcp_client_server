#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.h"
#include <math.h>

int main(int argc, char *argv[]) {
    int socket_desc;
    struct sockaddr_in server_addr;
    struct msg the_message;
    struct msg message_from_server;

    int externalIndex = atoi(argv[1]);
    float initialTemperature = atof(argv[2]);

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0) {
        printf("Unable to create socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Unable to connect\n");
        return -1;
    }

    printf("Connected with server successfully\n");
    printf("--------------------------------------------------------\n\n");

    int stable = 0;
    float oldTemp;
    float newTemp;

    the_message = prepare_message(externalIndex, initialTemperature, stable);
    while (1) {
        // Send message to server
        if (send(socket_desc, (const void *)&the_message, sizeof(the_message), 0) < 0) {
            printf("Unable to send message\n");
            return -1;
        }
        oldTemp = the_message.T;
        printf("old temp = %f\n", the_message.T);

        // Receive server's response
        if (recv(socket_desc, (void *)&message_from_server, sizeof(message_from_server), 0) < 0) {
            printf("Error while receiving server's msg\n");
            return -1;
        }
      
        newTemp = ((3 * oldTemp) + (2 * message_from_server.T))/5;
        printf("new temp = %f\n", newTemp);
        printf("\n");

        if (fabs(oldTemp - newTemp) <= 1) {
            stable = 1;
        }

        the_message = prepare_message(externalIndex, newTemp, stable);
    }

    return 0;
}
