#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "SocketClient.h"

int main(int argc, char* argv[]){
	int account;
	if (argc > 1)account = atoi(argv[1]);
	else scanf("%d", &account);
	bool log = true;
	if (argc > 2)log = atoi(argv[2]);
	SocketClient client(log);
	client.init(account);
	client.connect();
	int recver;
	char msg[MAX_MSG_LEN];
	while(client.isConnected()){
		scanf("%d %[^\n]", &recver, msg);
		client.send(recver, msg);
		memset(msg, 0, sizeof(msg));
	}
	return 0;
}