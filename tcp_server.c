#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "utils.h"

#define numExternals 4

int *establishConnectionsFromExternalProcesses() {
    // This socket is used by the server (i.e., Central process) to listen for 
    // connections from the External process. 
    int socket_desc;

    // Array containing the file descriptor of each server-client socket. 
    // There will be 4 client sockets, one for each external process. 
    //
    // Note that this array is declared as static so the function can return it 
    // to the caller function. A static int variable remains in memory while 
    // the program is running. A normal local variable is destroyed when a 
    // function call where the variable was declared returns. We want this 
    // array to persist.   
    static int client_socket[numExternals];
    unsigned int client_size;
    struct sockaddr_in server_addr, client_addr;

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        exit(0);
    }

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
        printf("Couldn't bind to the port\n");
        exit(0);
    }

    // Listen for clients:
    if(listen(socket_desc, 1) < 0){
        printf("Error while listening\n");
        exit(0);
    }

    printf("\n\nListening for incoming connections.....\n\n");
    printf("-------------------- Initial connections ---------------------------------\n");

    int externalCount = 0; 
    while (externalCount < numExternals){

        // Accept an incoming connection:
        client_socket[externalCount] = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
        
        if (client_socket[externalCount] < 0){
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("One external process connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        externalCount++; 
    }
    printf("--------------------------------------------------------------------------\n");
    printf("All four external processes are now connected\n");
    printf("--------------------------------------------------------------------------\n\n");

    return client_socket;   // Pointer to the array of file descriptors of client sockets 
}

int main(int argc, char *argv[]) {
    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    struct msg messageFromClient;   
    struct msg updatedMessage;
    float initialTemperature = atof(argv[1]);
    float centralTemp = initialTemperature;
    int *client_socket = establishConnectionsFromExternalProcesses(); 

    int stable = 0;
    while (stable == 0) {
        float temperature[numExternals];
        int stableCount = 0;

        for (int i = 0; i < numExternals; i++) {
            if (recv(client_socket[i], (void *)&messageFromClient, sizeof(messageFromClient), 0) < 0) {
                printf("Couldn't receive\n");
                return -1;
            }
            if (messageFromClient.stable == 1) 
                stableCount++;
            
            temperature[i] = messageFromClient.T;
            printf("Temperature of External Process (%d) = %.3f\n", i, messageFromClient.T);
        }

        if (stableCount == numExternals)
            stable = 1;

        float externalTempSum = temperature[0] + temperature[1] + temperature[2] + temperature[3];
        centralTemp = ((2 * centralTemp) + externalTempSum) / 6;    
       
        for (int i = 0; i < numExternals; i++) {
            updatedMessage = prepare_message(0, centralTemp, stable);
            if (send(client_socket[i], (const void *)&updatedMessage, sizeof(updatedMessage), 0) < 0) {
                printf("Can't send\n");
                return -1;
            }
        }        

        printf("\n");
    }  

    for (int i = 0; i < numExternals; i++) {
        close(client_socket[i]);
    }

    close(socket_desc);
    
    return 0;
}
