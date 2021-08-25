#ifndef SOCKETSERVER_H_
#define SOCKETSERVER_H_

#define MAX_MSG_LEN 1024
#define MAX_ACCOUNTS 1024
#define MAX_WAIT_MSG 32
#define MAX_WAIT_TIME 5

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
#include <iostream>

struct Node{
	int connfd;
	int account;
	pthread_t trans_thread;
	struct sockaddr_in sock_in;
};

struct TransPack{
	struct tm t;
	int sender, recver;
	char msg[MAX_MSG_LEN];
};

class SocketServer{
public:
	SocketServer();
	~SocketServer();
	bool init(const char* ip = "127.0.0.1", const int port = 1234);

private:
	static void* transmit(void* args);
	static void* connect(void* args);

private:
	static bool SearchInPool(int account, Node& node);
	static bool EraseFromPool(int account);
	static bool SearchInQueue(int account, std::list< std::list<TransPack> >::iterator& iter);
	static bool HangUpMsg(int account, TransPack& pack);

private:
	static int m_servfd;
	pthread_t conn_thread;
	static std::list<Node> g_conn_pool;
	static std::list< std::list<TransPack> > g_msg_queue;

};

#endif // !SOCKETSERVER_H_
