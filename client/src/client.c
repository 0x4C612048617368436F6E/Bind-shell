#include"../header/client.h"
#include<unistd.h>
#include <string.h>
void simpleClient(){
    CLIENT clientSocket = {0};
    clientSocket.socketFileDescriptor = socket(AF_INET,SOCK_STREAM,0);

    if(clientSocket.socketFileDescriptor == -1){
        perror("An error occured: ");
        exit(EXIT_FAILURE);
    }

    //server
    SERVERENDPOINT server = {0};
    server.socketAddress.sin_family = AF_INET;
    server.socketAddress.sin_port = htons(5000);
    server.socketAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(connect(clientSocket.socketFileDescriptor,(const struct sockaddr*)&server.socketAddress,sizeof(server.socketAddress)) == -1){
        perror("An error occured: ");
        exit(EXIT_FAILURE);
    }
    //recive
    simpleLogger("Enter input");
    char* buffer = NULL;
    ssize_t bytes = 0;
    ssize_t test = 0;
    char res[1024];
    size_t lengthOfUserInput = 0;
    size_t tempLengthOfUserInput = 0;
    while(1){
        bytes = 0;
        test = 0;
        printf("> ");
        //make sure to send the length of user input first
        buffer = processInput(&clientSocket.socketFileDescriptor);
        lengthOfUserInput = strlen(buffer);
        //network uses big endial
        tempLengthOfUserInput = lengthOfUserInput;
        lengthOfUserInput = htons(lengthOfUserInput);
        sendWrapper(clientSocket.socketFileDescriptor,(void*)&lengthOfUserInput,sizeof(int));
        //send actual message
        sendWrapper(clientSocket.socketFileDescriptor,(void*)buffer,tempLengthOfUserInput);
        //bytes = send(clientSocket.socketFileDescriptor,(void*)buffer,strlen(buffer)+1,0);
        //printf("Bytes sent is: %d\n",bytes);

        // test = recv(clientSocket.socketFileDescriptor,(void*)res,sizeof(res),0);
        // printf("Ok");
        // if(test == 0){
        //     printf("Server shutdown\n");
        // }else{
        //     printf("Bytes received: %d\n",test);
        //     printf("%s\n",res);
        // }
    }


    free(buffer);
    close(clientSocket.socketFileDescriptor);
}

char* processInput(int* clientFileDescriptor){
    char* buffer = malloc(sizeof(char)*SIZE_OF_INPUT_BUFFER);
    size_t pointer = 0;
    if(!buffer){
        perror("An error occured: ");
        close(*(clientFileDescriptor));
        exit(EXIT_FAILURE);
    }
    char c = '0';
    while(((c = getc(stdin)) != EOF) && (c != '\n')){
        //check if passed buffer boundary
        if(pointer >= SIZE_OF_INPUT_BUFFER){
            //realloc
            buffer = realloc(buffer,(sizeof(buffer) * SIZE_OF_INPUT_BUFFER)); //here
            if(!buffer){
                perror("An error occured: ");
                close(*(clientFileDescriptor));
                exit(EXIT_FAILURE);
            }

        }
        *(buffer+pointer++) = c;
    }

    if(pointer >= SIZE_OF_INPUT_BUFFER){ //-1 beacuse of null terminator
        buffer = realloc(buffer,(sizeof(buffer) * SIZE_OF_INPUT_BUFFER)); //here
        if(!buffer){
            perror("An error occured: ");
            close(*(clientFileDescriptor));
            exit(EXIT_FAILURE);
        }
    }
    *(buffer+pointer++) = '\0';
    return buffer;
}
//clientFileDescriptor is not the best name, but note its connected to the server
void sendWrapper(int clientFileDescriptor, void* buffer, size_t size){
    //convert to char
    char* pointerToBuffer = (char*)buffer;
    int bytesSent = 0;
    size_t remainingSizeToSend = size;
    ssize_t pointerToBufferToKeepTrackOfBytesSent = 0;
    while(remainingSizeToSend > 0){
        bytesSent = send(clientFileDescriptor,pointerToBuffer+pointerToBufferToKeepTrackOfBytesSent,remainingSizeToSend,0);
        pointerToBufferToKeepTrackOfBytesSent+=bytesSent;
        remainingSizeToSend-=bytesSent;
    }
    printf("Not greater than: \n");
}
