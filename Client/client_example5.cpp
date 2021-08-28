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
		sprintf(pack.msg, "%s/%s", "userE", "123456abc");
		client.send(pack);
		while(client.get(pack) == -1); // get id
		printf("%d\n", pack.recver);*/

		/* Login */
		pack.id = R_LOGIN;
		pack.sender = 5;
		pack.recver = SERVER_IDX;
		sprintf(pack.msg, "%s", "123456abc");
		client.send(pack);
		memset(pack.msg, 0, strlen(pack.msg));
		while(client.get(pack) == -1); // get name
		printf("username = %s\n", pack.msg);

		/* send image */
		FILE *fq;
		if((fq = fopen("img.png","rb")) == NULL){
        	printf("Failed to open image.\n");
        	return 0;
    	}
		pack.id = D_FILE_PACK; // image_id
		pack.sender = 5;
		pack.recver = 6;
		time_t timer;
		time(&timer);
		pack.t = *localtime(&timer); // very important
		int ctr = 0;
		while(!feof(fq)){
			memset(pack.msg, 0, sizeof(pack.msg));
			int len = fread(pack.msg, 1, sizeof(pack.msg), fq);
			client.send(pack);
			ctr++;
		}
		printf("size=%d\n", ctr);
		fclose(fq);
		pack.id = D_FILE_END;
		memset(pack.msg, 0, sizeof(pack.msg));
		sprintf(pack.msg, "%d", ctr);
		client.send(pack);
	}
	while(1);
	return 0;
}