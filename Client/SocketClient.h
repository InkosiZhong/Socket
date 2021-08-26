#ifndef SOCKETCLIENT_H_
#define SOCKETCLIENT_H_
#define WIN32

#define MAX_MSG_LEN 1024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __linux__
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#endif
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <thread> // c++11
#pragma comment(lib, "ws2_32.lib")
#endif
#include <time.h>
#include <list>

struct TransPack{
	int sender, recver;
	char msg[MAX_MSG_LEN];
	struct tm t;
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
	static void* _receive(void* args);
	static void* _send(void* args);

private:
#ifdef __linux__
	pthread_t recv_thread, send_thread;
#endif
#ifdef WIN32
	std::thread recv_thread, send_thread;
#endif
	Args args;
	static bool m_log;
	static std::list<TransPack> send_buffer, recv_buffer;

};

#endif // !SOCKETCLIENT_H_
