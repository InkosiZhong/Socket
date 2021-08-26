#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "SocketServer.h"

int main(){
	SocketServer server;
    server.init("192.168.31.186");
    return 0;
}