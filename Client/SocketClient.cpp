#include "SocketClient.h"

bool SocketClient::m_log = true;
std::list<TransPack> SocketClient::send_buffer = std::list<TransPack>();
std::list<TransPack> SocketClient::recv_buffer = std::list<TransPack>();

void* SocketClient::_receive(void* args){
	Args& targs = *(Args*)args;
	TransPack pack;
	while(1){
		memset(pack.msg, 0, strlen(pack.msg) + 1);
#if defined(__APPLE__) || defined(__linux__)
		usleep(1e4); // us
#elif defined(_WIN32)
		Sleep(10); // ms
#endif
		int ret = recv(targs.connfd, (char*)&pack, sizeof(pack), 0);
        if (ret > 0){
			printf("### receive form %d: %s ｜ %s\n", pack.sender, pack.msg, asctime(&pack.t));
			recv_buffer.push_back(pack);
		}
		else if (ret < 0){
			if (m_log)printf("### Failed to receive message(%d)\n", ret);
			targs.connected = false;
			break;
		}
		else {
			if (m_log)printf("### Connection break.\n");
			targs.connected = false;
			break;
		}
	}
	targs.connected = false;
#if defined(__APPLE__) || defined(__linux__)
	pthread_exit(NULL);
#elif defined(_WIN32)
	return NULL;
#endif
}

bool SocketClient::send(int recver, const char* msg){
	time_t timer;
	time(&timer);
	TransPack pack;
	memset(pack.msg, 0, sizeof(pack.msg));
	pack.sender = args.account;
	pack.recver = recver;
	pack.t = *localtime(&timer);
	memcpy(pack.msg, msg, strlen(msg));
	send_buffer.push_back(pack);
	return isConnected();
}

bool SocketClient::send(TransPack& pack){
	send_buffer.push_back(pack);
	return isConnected();
}

int SocketClient::get(TransPack& pack, int order){
	if (recv_buffer.empty())return -1;
	if (order == FRONT){
		pack = recv_buffer.front();
		recv_buffer.pop_front();
	}
	else {
		pack = recv_buffer.back();
		recv_buffer.pop_back();
	}
	return recv_buffer.size();
}

int SocketClient::get(std::list<TransPack>& list){
	if (recv_buffer.empty())return -1;
	int size = recv_buffer.size();
	list = recv_buffer;
	recv_buffer.clear();
	return size;
}

void* SocketClient::_send(void* args){
	Args& targs = *(Args*)args;
	TransPack pack;
	pack.sender = targs.account;
	while(targs.connected){
#if defined(__APPLE__) || defined(__linux__)
		usleep(1e4);
#elif defined(_WIN32)
		Sleep(10);
#endif
		while (!send_buffer.empty()){
			pack = send_buffer.front(); 
			send_buffer.pop_front();
			int ret = ::send(targs.connfd, (char*)&pack, sizeof(pack), 0);
			if (ret < 0) {
				if (m_log)printf("### Failed to send message(%d)\n", ret);
				targs.connected = false;
				break;
			}
			else if (ret == 0){
				if (m_log)printf("### Connection break.\n");
				targs.connected = false;
				break;
			}
		}
	}
#if defined(__APPLE__) || defined(__linux__)
	pthread_exit(NULL);
#elif defined(_WIN32)
	return NULL;
#endif
}

SocketClient::SocketClient(bool log) { 
	m_log = log; 
	args.connected = false;
}

SocketClient::~SocketClient(){
#if defined(__APPLE__) || defined(__linux__)
	pthread_cancel(recv_thread);
	pthread_cancel(send_thread);
	close(args.connfd);
#elif defined(_WIN32)
	closesocket(args.connfd);
	WSACleanup();
#endif
}

bool SocketClient::connect(const char* ip, const int port){
	int ret = 0;
#if defined(_WIN32)
	WSADATA wsaData;
	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0) {
		printf("Failed to load Winsock\n");
		return false;
	}
#endif
	//创建套接字
    args.connfd = socket(AF_INET, SOCK_STREAM, 0);

    //向服务器（特定的IP和端口）发起请求
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr(ip);  //具体的IP地址
    serv_addr.sin_port = htons(port);  //端口
    ret = ::connect(args.connfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret != 0){
		printf("### Failed to connect: error_code = %d\n", ret);
		return false;
	}

	args.connected = true;

#if defined(__APPLE__) || defined(__linux__)
	ret = pthread_create(&recv_thread, NULL, _receive, (void*)&(args));
	if (ret != 0) {
		if (m_log)printf("### pthread_create for recv_thread error: error_code = %d\n", ret);
		return false;
	}
    pthread_detach(recv_thread);
	ret = pthread_create(&send_thread, NULL, _send, (void*)&(args));
	if (ret != 0) {
		if (m_log)printf("### pthread_create for send_thread error: error_code = %d\n", ret);
		return false;
	}
    pthread_detach(send_thread);
#elif defined(_WIN32)
	recv_thread = std::thread(_receive, &args);
	send_thread = std::thread(_send, &args);
	recv_thread.detach();
	send_thread.detach();
#endif
	while(!args.connected);
	//while(args.connected);
	return true;
}

bool SocketClient::isConnected(){ return args.connected; }