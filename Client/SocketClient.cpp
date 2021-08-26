#include "SocketClient.h"

bool SocketClient::m_log = true;
std::list<TransPack> SocketClient::send_buffer = std::list<TransPack>();
std::list<TransPack> SocketClient::recv_buffer = std::list<TransPack>();

void* SocketClient::_receive(void* args){
	Args& targs = *(Args*)args;
	TransPack pack;
	while(1){
		memset(pack.msg, 0, strlen(pack.msg) + 1);
#ifdef __linux__
		usleep(1e4); // us
#endif
#ifdef WIN32
		Sleep(10); // ms
#endif
		int ret = recv(targs.connfd, (char*)&pack, sizeof(pack), 0);
        if (ret > 0){
			printf("receive form %d: %s ｜ %s\n", pack.sender, pack.msg, asctime(&pack.t));
			if (!targs.connected && pack.sender == -1){
				targs.connected = true;
			}
			else {
				recv_buffer.push_back(pack);
			}
		}
		else if (ret < 0){
			if (m_log)printf("### Failed to receive message(%d)\n", ret);
			break;
		}
		else {
			if (m_log)printf("### Connection break.\n");
			break;
		}
	}
	targs.connected = false;
#if __linux__
	pthread_exit(NULL);
#endif
#ifdef WIN32
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
	return true;
}

bool SocketClient::send(TransPack& pack){
	send_buffer.push_back(pack);
	return true;
}

int SocketClient::get(TransPack& pack){
	if (recv_buffer.empty())return -1;
	pack = recv_buffer.front();
	recv_buffer.pop_front();
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
	/* transmit account */
	while(!targs.connected){
		if (m_log)printf("### waiting for response...\n");
		memset(pack.msg, 0, strlen(pack.msg) + 1);
		pack.recver = -1;
#if __linux__
		usleep(1e5);
#endif
#ifdef WIN32
		Sleep(100);
#endif
		time_t timer;
		time(&timer);
		pack.t = *localtime(&timer);
		printf("send form %d to %d: %s | %s\n", pack.sender, pack.recver, pack.msg, asctime(&pack.t));
		int ret = ::send(targs.connfd, (char*)&pack, sizeof(pack), 0);
		if (ret < 0){
			if (m_log)printf("### Failed to send account(%d)\n", ret);
			break;
		}
        else if (ret == 0){
			if (m_log)printf("### Connection break.\n");
			break;
		}
	}
	if (m_log)printf("### Connection established with server\n");
	while(targs.connected){
#if __linux__
		usleep(1e4);
#endif
#ifdef WIN32
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
#ifdef __linux__
	pthread_exit(NULL);
#endif
#ifdef WIN32
	return NULL;
#endif
}

SocketClient::SocketClient(bool log) { m_log = log; }

SocketClient::~SocketClient(){
#ifdef __linux__
	pthread_cancel(recv_thread);
	pthread_cancel(send_thread);
	close(args.connfd);
#endif
#ifdef WIN32
	closesocket(args.connfd);
	WSACleanup();
#endif
}

bool SocketClient::init(int account){
	args.account = account;
	return true;
}

bool SocketClient::connect(const char* ip, const int port){
#ifdef WIN32
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
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
    ::connect(args.connfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	
	args.connected = false;

#ifdef __linux__
	int ret = pthread_create(&recv_thread, NULL, receive, (void*)&(args));
	if (ret != 0) {
		if (m_log)printf("### pthread_create for recv_thread error: error_code = %d\n", ret);
		return false;
	}
    pthread_detach(recv_thread);
	ret = pthread_create(&send_thread, NULL, send, (void*)&(args));
	if (ret != 0) {
		if (m_log)printf("### pthread_create for send_thread error: error_code = %d\n", ret);
		return false;
	}
    pthread_detach(send_thread);
#endif
#ifdef WIN32
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