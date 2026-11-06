#include"./common.h"
#include<sys/socket.h>
#include<arpa/inet.h>
#define DEFAULT_BUFFER_SIZE 1024
typedef struct{
    int clientFileDescriptor;
    //client socket
    struct sockaddr client;
    //bot -> points to client socket too
    struct sockaddr bot;
}CLIENTENDPOINT;

typedef struct{
    int socketFileDescriptor;
    struct sockaddr_in socketAddress;
}SERVER;
void simpleServer();
void recvWrapper(int, void*, size_t);
void sendWrapper(int, void*, size_t);
void writeWrapper(int, void*, size_t);
void readWrapper(int, void*, size_t);
size_t customMax(int, int);