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
		sprintf(pack.msg, "%s/%s", "userB", "123456abc");
		client.send(pack);
		while(client.get(pack) == -1); // get id
		printf("%d\n", pack.recver);*/

		/* Login */
		pack.id = R_LOGIN;
		pack.sender = 2;
		pack.recver = SERVER_IDX;
		sprintf(pack.msg, "%s", "123456abc");
		client.send(pack);
        memset(pack.msg, 0, strlen(pack.msg));
		while(client.get(pack) == -1); // get name
		printf("username = %s\n", pack.msg);

		/* reply friend applicant */
        while(client.get(pack) == -1); // get friend applicant
        int request_id = atoi(pack.msg);
        printf("request_id = %d\n", request_id);
		pack.id = R_REPLY;
        pack.sender = 2;
        pack.recver = SERVER_IDX;
        sprintf(pack.msg, "%d/%d", request_id, 1);
        client.send(pack);
        memset(pack.msg, 0, strlen(pack.msg));
        while(client.get(pack) == -1); // get friend
        printf("friend = %s\n", pack.msg);

        /* chat */
        while(client.get(pack) == -1); // get message
        printf("%s at %s", pack.msg, asctime(&pack.t));
	}

    while(1);
	return 0;
}