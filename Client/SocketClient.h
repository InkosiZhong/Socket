#ifndef SOCKETCLIENT_H_
#define SOCKETCLIENT_H_

#define MAX_MSG_LEN 1024
enum Order{FRONT, BACK};
enum REQUEST_CODE{R_REGISTER = -1, R_LOGIN = -2, R_APPLICANT = -3, R_REPLY = -4, R_UPDATEPWD = -5, R_UPDATEALIAS = -6};
enum DATA_CODE{D_ME = -11, D_FRIEND = -22, D_FILE_PACK = -33, D_FILE_END = -44};
#define SERVER_IDX -1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#include <thread> // c++11
#pragma comment(lib, "ws2_32.lib")
#endif
#include <time.h>
#include <list>

struct TransPack{
	int id;
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
	/** SocketClient
	 * @brief 构造SocketClient对象
	 * @param log: bool(true) 是否显示调试记录
	 * @return SocketClient对象
	*/
	SocketClient(bool log = true);
	~SocketClient();

	/** connect
	 * @brief 建立Socket连接
	 * @param ip: const char*("127.0.0.1") 服务器ip
	 * @param port: const int(1234) 服务器端口
	 * @return bool 是否连接
	*/
	bool connect(const char* ip = "127.0.0.1", const int port = 1234);

	/** send - 不推荐
	 * @brief 发送chat record，自动生成时间戳
	 * @param recver: int 收信账户
	 * @param msg: const char* 消息
	 * @return bool 是否成功发送
	*/
	bool send(int recver, const char* msg);

	/** send
	 * @brief 发送Transmition Package
	 * @param pack: TransPack& Transmition Package
	 * @return bool 是否成功发送
	*/
	bool send(TransPack& pack);

	/** get
	 * @brief 获取单个待读Transmition Package
	 * @param pack: TransPack& Transmition Package
	 * @param order: int(FRONT), 获取顺序(FRONT最早, BACK最新)
	 * @return int 成功: buffer内仍待读的包数, 失败: -1
	*/
	int get(TransPack& pack, int order = FRONT);

	/** get
	 * @brief 获取全部待读Transmition Package
	 * @param pack: std::list<TransPack>& Transmition Package队列
	 * @return int 成功: 此次获取到的包数, 失败: -1
	*/
	int get(std::list<TransPack>& list);

	/** isConnected
	 * @brief 获取连接状态
	 * @return bool 连接状态
	*/
	bool isConnected();

private:
	static void* _receive(void* args);
	static void* _send(void* args);

private:
#if defined(__APPLE__) || defined(__linux__)
	pthread_t recv_thread, send_thread;
#elif defined(_WIN32)
	std::thread recv_thread, send_thread;
#endif
	Args args;
	static bool m_log;
	static std::list<TransPack> send_buffer, recv_buffer;

};

#endif // !SOCKETCLIENT_H_
