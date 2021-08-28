#include <stdio.h>
#include "SocketClient.h"

int main(int argc, char* argv[]){
	bool log = true;
	if (argc > 1)log = atoi(argv[1]);
	SocketClient client(log);
	client.connect();
	int recver;
	char msg[MAX_MSG_LEN];
	TransPack pack;
	if (client.isConnected()){
		/* Register */
		/*pack.id = R_REGISTER;
		pack.sender = 0;
		pack.recver = SERVER_IDX;
		sprintf(pack.msg, "%s/%s", "userA", "123456abc");
		client.send(pack);
		while(client.get(pack) == -1); // get id
		printf("%d\n", pack.recver);*/

		/* Login */
		pack.id = R_LOGIN;
		pack.sender = 1;
		pack.recver = SERVER_IDX;
		sprintf(pack.msg, "%s", "123456abc");
		client.send(pack);
		memset(pack.msg, 0, strlen(pack.msg));
		while(client.get(pack) == -1); // get name
		printf("username = %s\n", pack.msg);

		/* create friend applicant */
		pack.id = R_APPLICANT;
		pack.sender = 1;
		pack.recver = 2;
		client.send(pack);
		memset(pack.msg, 0, strlen(pack.msg));
		while(client.get(pack) == -1); // get friend
		printf("friend = %s\n", pack.msg);

		/* chat */
		pack.id = 0;
		pack.sender = 1;
		pack.recver = 2;
		sprintf(pack.msg, "helloworld");
		time_t timer;
		time(&timer);
		pack.t = *localtime(&timer);
		client.send(pack);
		memset(pack.msg, 0, strlen(pack.msg));
		
	}
	while(1);
	return 0;
}