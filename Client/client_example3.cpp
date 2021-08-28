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
		sprintf(pack.msg, "%s/%s", "userC", "123456abc");
		client.send(pack);
		while(client.get(pack) == -1); // get id
		printf("%d\n", pack.recver);*/

		/* Login */
		pack.id = R_LOGIN;
		pack.sender = 3;
		pack.recver = SERVER_IDX;
		sprintf(pack.msg, "%s", "123456abc");
		client.send(pack);
		memset(pack.msg, 0, strlen(pack.msg));
		while(client.get(pack) == -1); // get name
		printf("username = %s\n", pack.msg);

		/* offline chat (make sure 3 and 4 are friend of each other) */
		for (int i = 0; i < 25; i++){
			pack.id = 0;
			pack.sender = 3;
			pack.recver = 4;
			sprintf(pack.msg, "helloworld%d", i);
			time_t timer;
			time(&timer);
			pack.t = *localtime(&timer);
			client.send(pack);
			memset(pack.msg, 0, strlen(pack.msg));
		}

		/* change pwd and alias */
		pack.id = R_UPDATEPWD;
		pack.sender = 3;
		pack.recver = SERVER_IDX;
		sprintf(pack.msg, "%s", "123456abc/123456");
		client.send(pack);
		memset(pack.msg, 0, strlen(pack.msg));
		pack.id = R_UPDATEALIAS;
		pack.sender = 3;
		pack.recver = 4;
		sprintf(pack.msg, "%s", "friend4");
		client.send(pack);
		memset(pack.msg, 0, strlen(pack.msg));
	}
	while(1);
	return 0;
}