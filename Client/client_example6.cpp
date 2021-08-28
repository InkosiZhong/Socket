#include <stdio.h>
#include "SocketClient.h"

struct tpsort{
	bool operator () (const TransPack& tp1, const TransPack& tp2){
		struct tm t1 = tp1.t, t2 = tp2.t;
		return difftime(mktime(&t1), mktime(&t2)) < 0;
	}
};

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
		sprintf(pack.msg, "%s/%s", "userF", "123456abc");
		client.send(pack);
		while(client.get(pack) == -1); // get id
		printf("%d\n", pack.recver);*/

		/* Login */
		pack.id = R_LOGIN;
		pack.sender = 6;
		pack.recver = SERVER_IDX;
		sprintf(pack.msg, "%s", "123456abc");
		client.send(pack);
        memset(pack.msg, 0, strlen(pack.msg));
		while(client.get(pack) == -1); // get name
		printf("username = %s\n", pack.msg);

		/* receive image */
		FILE* fp = fopen("new_img.png","ab");
		std::list<TransPack> list;
		while(1){
			while(client.get(pack) == -1);
			if (pack.id == D_FILE_PACK){
				list.push_back(pack);
			}
			else if (pack.id == D_FILE_END){
				if (atoi(pack.msg) == list.size()){
					list.sort(tpsort());
					break;
				}
				else{
					printf("incorrect pack size\n");
					return 0;
				}
			}
		}
		printf("list.size()=%d\n", int(list.size()));
		while(!list.empty()){
			fwrite(list.front().msg, 1, sizeof(list.front().msg), fp);
			list.pop_front();
		}
		fclose(fp);
	}

    while(1);
	return 0;
}