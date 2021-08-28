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
		sprintf(pack.msg, "%s/%s", "userD", "123456abc");
		client.send(pack);
		while(client.get(pack) == -1); // get id
		printf("%d\n", pack.recver);*/

		/* Login */
		pack.id = R_LOGIN;
		pack.sender = 4;
		pack.recver = SERVER_IDX;
		sprintf(pack.msg, "%s", "123456abc");
		client.send(pack);
        memset(pack.msg, 0, strlen(pack.msg));
		while(client.get(pack) == -1); // get name
		printf("username = %s\n", pack.msg);

		/* get friend 3 */
		while(client.get(pack) == -1);

        /* get offline chat records */
		TransPack the_pack;
		for (int i = 0; i < 25; i++){
			if (i == 15){
				while(client.get(the_pack) == -1); // get message
				printf("(%d,id=%d) %s at %s", i, the_pack.id, the_pack.msg, asctime(&the_pack.t));
			}
			else{
				while(client.get(pack) == -1); // get message
				printf("(%d,id=%d) %s at %s", i, pack.id, pack.msg, asctime(&pack.t));
			}
		}
		printf("----------------------------\n");
		/* get 10 historical records before the given id */
		pack.id = the_pack.id;
		pack.sender = 4;
		pack.recver = SERVER_IDX;
		memset(pack.msg, 0, strlen(pack.msg));
		sprintf(pack.msg, "%d", 3);
		client.send(pack);
		for (int i = 0; i < 10; i++){
			while(client.get(pack) == -1); // get message
			printf("(id=%d) %s at %s", pack.id, pack.msg, asctime(&pack.t));
		}
	}

    while(1);
	return 0;
}