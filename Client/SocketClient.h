#ifndef SOCKETCLIENT_H_
#define SOCKETCLIENT_H_

#define MAX_MSG_LEN 1024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <list>

struct TransPack{
	struct tm t;
	int sender, recver;
	char msg[MAX_MSG_LEN];
};

struct Args{
	int connfd;
	int account;
	bool connected;
};

class SocketClient{
public:
	SocketClient(bool log = true);
	~SocketClient();
	bool init(int account);
	bool connect(const char* ip = "127.0.0.1", const int port = 1234);
	bool send(int recver, const char* msg);
	bool send(TransPack& pack);
	int get(TransPack& pack);
	int get(std::list<TransPack>& list);
	bool isConnected();

private:
	static void* receive(void* args);
	static void* send(void* args);

private:
	pthread_t recv_thread, send_thread;
	Args args;
	static bool m_log;
	static std::list<TransPack> send_buffer, recv_buffer;

};

#endif // !SOCKETCLIENT_H_
