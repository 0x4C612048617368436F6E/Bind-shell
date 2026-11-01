#include"./common.h"
#include<sys/socket.h>
#include<arpa/inet.h>
#define SIZE_OF_INPUT_BUFFER 1024
typedef struct{
    int socketFileDescriptor;
    struct sockaddr_in clientAddress;
}CLIENT;

typedef struct{
    //server socket
    struct sockaddr_in socketAddress;
}SERVERENDPOINT;
void simpleClient();
char* processInput(int*);
void sendWrapper(int, void*, size_t);