#include"../header/server.h"
#include<unistd.h>
#include<string.h>
#include<pty.h>
void simpleServer(){
    simpleLogger("Initilising server");
    SERVER serverSocket = {0};
    serverSocket.socketFileDescriptor = socket(AF_INET,SOCK_STREAM,0);
    if(serverSocket.socketFileDescriptor == -1){
        perror("An error occured: ");
        exit(EXIT_FAILURE);
    }
    //bind socket
    serverSocket.socketAddress.sin_family = AF_INET;
    serverSocket.socketAddress.sin_port = htons(5000);
    serverSocket.socketAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(bind(serverSocket.socketFileDescriptor,(const struct sockaddr*)&serverSocket.socketAddress,(socklen_t)sizeof(serverSocket.socketAddress)) == -1){
        perror("An error occured: ");
        close(serverSocket.socketFileDescriptor);
        exit(EXIT_FAILURE);
    }

    simpleLogger("Simple server is listening");
    if(listen(serverSocket.socketFileDescriptor,5) == -1){
        perror("An error occused: ");
        close(serverSocket.socketFileDescriptor);
        exit(EXIT_FAILURE);
    }

    //accept client
    CLIENTENDPOINT client = {0};
    client.clientFileDescriptor = 0;
    socklen_t addrlen = sizeof(client.client);
    if((client.clientFileDescriptor = accept(serverSocket.socketFileDescriptor,(struct sockaddr*)&(client.client),(socklen_t*)&addrlen)) == -1){
        perror("An error occused: ");
        close(serverSocket.socketFileDescriptor);
        exit(EXIT_FAILURE);
    }
    simpleLogger("Attack connected");
    //set the bot to the client
    client.bot = client.client;

    //create a new process to run our shell
    int amaster, aslave;
    char name[1024];
    struct termios termp = {0};
    struct winsize winp = {0};
    if(openpty(&amaster,&aslave,name,&termp,&winp) == -1){
        perror("An error occused: ");
        close(serverSocket.socketFileDescriptor);
         close(client.clientFileDescriptor);
        exit(EXIT_FAILURE);
    }
    int forkRet = fork();
    if(forkRet == 0){
        //child process
        //close master of child process
        close(amaster);
        if(setsid() == -1){
            perror("An error occured: ");
            close(serverSocket.socketFileDescriptor);
         close(client.clientFileDescriptor);
            close(aslave);
            exit(EXIT_FAILURE);
        }
        if(ioctl(aslave,TIOCSCTTY) == -1){
            perror("An error occured: ");
            close(serverSocket.socketFileDescriptor);
         close(client.clientFileDescriptor);
            close(aslave);
            exit(EXIT_FAILURE);
        }
        //attach stdin,stdout and stderr to the child
        for(int i = 0; i<3; i++){
            if(dup2(aslave,i) == -1){
                perror("An error occured: ");
                close(serverSocket.socketFileDescriptor);
         close(client.clientFileDescriptor);
                close(aslave);
                exit(EXIT_FAILURE);
            }
        }

        //run bash
        char *args[] = { "bash", NULL };
        char *env[] = { NULL };

        if(execve("/usr/bin/bash", args, env) == -1){
            close(serverSocket.socketFileDescriptor);
            //i guess have to close client descript on our end
            close(client.clientFileDescriptor);
         close(client.clientFileDescriptor);
            perror("An error occured: ");
            exit(EXIT_FAILURE);
        }

    }
    if(forkRet < 0){
        //failure
        perror("An error occured: ");
        close(serverSocket.socketFileDescriptor);
        exit(EXIT_FAILURE);
    }
    //close aslave as not needed in parent process
    close(aslave);
    //running the main server
    char* buffer = (char*)malloc(sizeof(char)*DEFAULT_BUFFER_SIZE);
    if(!buffer){
        perror("An error occured: ");
        close(serverSocket.socketFileDescriptor);
        //close(amaster);
        exit(EXIT_FAILURE);
    }
    size_t sizeToMakeActualMessageBe = 0;
    size_t lengthOfStdoutOutput = 0;
    int justStarted = 0;
    size_t lengthOfUserInput = 0;
    while(1){

        //will read the startup first
        if(justStarted == 0){
            int lengthOfStartUpScript = 0;
            lengthOfStdoutOutput = 1024;
            //shift file pointer indicator back
            //fseek(aslave,0,SEEK_SET);
            buffer = (char*)realloc(buffer,lengthOfStdoutOutput);
            if(!buffer){
                perror("An error occused: ");
                close(serverSocket.socketFileDescriptor);
                free(buffer);
                exit(EXIT_FAILURE);
            }
            //printf("Reading from amster\n");
            readWrapper(amaster,(void*)buffer,lengthOfStdoutOutput);
            //printf("Finished amaster\n");
            *(buffer+strlen(buffer)) = '\0';
            //simpleLogger(buffer);
            lengthOfUserInput = strlen(buffer);
            //network uses big endial
            lengthOfStdoutOutput = lengthOfUserInput;
            lengthOfUserInput = htons(lengthOfUserInput);
            sendWrapper(client.clientFileDescriptor, (void*)&lengthOfUserInput, sizeof(int));
            sendWrapper(client.clientFileDescriptor,(void*)buffer,lengthOfStdoutOutput);
            justStarted+=1;
            continue;
        }

        //first recv from client - The size of the message first (will limit it to int which is 4 bytes)
        //realloc back
        lengthOfStdoutOutput = 0;
        buffer = (char*)realloc(buffer,sizeof(int));
        if(!buffer){
            perror("An error occused: ");
            close(serverSocket.socketFileDescriptor);
            free(buffer);
            exit(EXIT_FAILURE);
        }
        memset(buffer,0,sizeof(int));
        //printf("Waiting\n");
        recvWrapper(client.clientFileDescriptor,(void*)buffer,sizeof(int));
        //printf("Wanting ti receive\n");
        //convert into integer type
        char* pointerToSizeToMakeActualMessageBe = (char*)((void*)&sizeToMakeActualMessageBe);
        for(int i = 0; i<sizeof(int); i++){
            *(pointerToSizeToMakeActualMessageBe+i) = *(buffer+i);
        }
        //keep in mind network uses big endian
        sizeToMakeActualMessageBe = ntohs(sizeToMakeActualMessageBe);
        //printf("Size of message is: %ld\n",sizeToMakeActualMessageBe);
        //now we get the actual client message - realloc based on the size of the message
        //account for new line here as we did not in the client
        buffer = (char*)realloc(buffer,sizeToMakeActualMessageBe+1);
        if(!buffer){
            perror("An error occused: ");
            close(serverSocket.socketFileDescriptor);
            free(buffer);
            exit(EXIT_FAILURE);
        }
        recvWrapper(client.clientFileDescriptor,(void*)buffer,sizeToMakeActualMessageBe);
        *(buffer + sizeToMakeActualMessageBe) = '\0';
        printf("%s\n",buffer);
    
        //then send to amaster (the PTY interface between)
        //printf("Writing from amaster\n");
        writeWrapper(amaster,(void*)buffer,sizeToMakeActualMessageBe+1);
        writeWrapper(amaster, (void*)"\n", 1);
        //printf("Finished amaster\n");
        //calculate size of stdout output
        //fseek(aslave,0,SEEK_END);
        lengthOfStdoutOutput = 1024;
        //shift file pointer indicator back
        //fseek(aslave,0,SEEK_SET);
        buffer = (char*)realloc(buffer,lengthOfStdoutOutput);
        if(!buffer){
            perror("An error occused: ");
            close(serverSocket.socketFileDescriptor);
            free(buffer);
            exit(EXIT_FAILURE);
        }
        //printf("Reading from amster\n");
        readWrapper(amaster,(void*)buffer,lengthOfStdoutOutput);
        //printf("Finished amaster\n");
        //send length of message
        *(buffer+strlen(buffer)) = '\0';
        lengthOfUserInput = strlen(buffer);
        //network uses big endial
        lengthOfStdoutOutput = lengthOfUserInput;
        lengthOfUserInput = htons(lengthOfUserInput);
        sendWrapper(client.clientFileDescriptor, (void*)&lengthOfUserInput, sizeof(int));

        //send actual message
        sendWrapper(client.clientFileDescriptor,(void*)buffer,lengthOfStdoutOutput);
            justStarted+=1;


    }
}

void recvWrapper(int clientFileDescriptor, void* buffer, size_t size){
    //convert to char
    char* pointerToBuffer = (char*)buffer;
    int bytesReceived = 0;
    ssize_t pointerToBufferToKeepTrackOfBytesReceived = 0;
    size_t remainingSizeToReceive = size;
    while(remainingSizeToReceive > 0){
        bytesReceived = recv(clientFileDescriptor,pointerToBuffer+pointerToBufferToKeepTrackOfBytesReceived,remainingSizeToReceive,0);
        pointerToBufferToKeepTrackOfBytesReceived+=bytesReceived;
        remainingSizeToReceive-=bytesReceived;
    }
}

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
    //printf("Not greater than: \n");
}

void writeWrapper(int amasterFileDescriptor, void* buffer, size_t size){
    //convert to char
    char* pointerToBuffer = (char*)buffer;
    int bytesSent = 0;
    size_t remainingSizeToSend = size;
    ssize_t pointerToBufferToKeepTrackOfBytesSent = 0;
    while(remainingSizeToSend > 0){
        bytesSent = write(amasterFileDescriptor,pointerToBuffer+pointerToBufferToKeepTrackOfBytesSent,remainingSizeToSend);
        pointerToBufferToKeepTrackOfBytesSent+=bytesSent;
        remainingSizeToSend-=bytesSent;
    }
}

void readWrapper(int amasterFileDescriptor, void* buffer, size_t size){
    //convert to char
    int isThereMoreBytes = 0;
    if (ioctl(amasterFileDescriptor, FIONREAD, &isThereMoreBytes) == -1) {
        printf("NO MORE BYTES TO BE READ\n");
    }
    char* pointerToBuffer = (char*)buffer;
    ssize_t pointerToBufferToKeepTrackOfBytesReceived = 0;
    size_t remainingSizeToReceive = size; 
    int bytesReceived = read(amasterFileDescriptor,pointerToBuffer+pointerToBufferToKeepTrackOfBytesReceived,remainingSizeToReceive);
    //printf("Bytes recieved: %d\n",bytesReceived);
    //need the below just to test if new byteReeived is less than, so that we can just stop
    printf("MORE BYTES TO BE READ\n");
    printf("Values of isThereMoreBytes Up: %d\n",isThereMoreBytes);

    while(isThereMoreBytes > 0){
        printf("Reading bytes\n");
        //previousBytesReceived = bytesReceived;
        pointerToBufferToKeepTrackOfBytesReceived+=bytesReceived;
        //printf("%s",pointerToBuffer);
        //printf("Attempting to read\n");
        //printf("Bytes recieved: %d\n",bytesReceived);

        bytesReceived = read(amasterFileDescriptor,pointerToBuffer+pointerToBufferToKeepTrackOfBytesReceived,remainingSizeToReceive);
        //remainingSizeToReceive-=bytes;
        if (ioctl(amasterFileDescriptor, FIONREAD, &isThereMoreBytes) == -1) {
            printf("NO MORE BYTES TO BE READ\n");
        }
        printf("Bytes received: %d\n",bytesReceived);
        printf("Values of isThereMoreBytes Down: %d\n",isThereMoreBytes);
        //get the difference between bytes received and isThereMoreBytes and use that to get the remaining bytes
        
    }
}

size_t customMax(int a, int b){
    return (a > b) ? (size_t)((size_t)a - (size_t)b) : (size_t)((size_t)b - (size_t)a);
}