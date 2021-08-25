#include "SocketServer.h"

int SocketServer::m_servfd = -1;
std::list<Node> SocketServer::g_conn_pool = std::list<Node>();
std::list< std::list<TransPack> > SocketServer::g_msg_queue = std::list< std::list<TransPack> >();

bool SocketServer::SearchInPool(int account, Node& node){
	std::list<Node>::iterator iter = g_conn_pool.begin();
	while(iter != g_conn_pool.end()){
		if (iter->account == account){
			node = *iter;
			return true;
		}
		iter++;
	}
	return false;
}

bool SocketServer::EraseFromPool(int account){
	std::list<Node>::iterator iter = g_conn_pool.begin();
	while(iter != g_conn_pool.end()){
		if (iter->account == account){
			g_conn_pool.erase(iter);
			printf("### Connection from %d shutdown.\n", account);
			return true;
		}
		iter++;
	}
	return false;
}

bool SocketServer::SearchInQueue(int account, std::list< std::list<TransPack> >::iterator& iter){
	iter = g_msg_queue.begin();
	while(iter != g_msg_queue.end()){
		while (iter->empty()){
			iter = g_msg_queue.erase(iter);
			if (iter == g_msg_queue.end())return false;
		}
		if (iter->begin()->recver == account){
			return true;
		}
		iter++;
	}
	return false;
}

bool SocketServer::HangUpMsg(int account, TransPack& pack){
	std::list< std::list<TransPack> >::iterator iter;
	if (SearchInQueue(account, iter)){ // in queue
		iter->push_back(TransPack(pack));
	}
	else { // not in queue
		std::list<TransPack> new_queue;
		new_queue.push_back(TransPack(pack));
		g_msg_queue.push_back(new_queue);
	}
	return true;
}

void* SocketServer::transmit(void* args){
	Node node = *(Node*)args;
	TransPack pack;
	while(1){
		usleep(1e4);
		memset(pack.msg, 0, sizeof(pack.msg));
		int ret = read(node.connfd, (char*)&pack, sizeof(pack));
        if (ret > 0){
			printf("receive \"%s\" | form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
			Node recv_node;
			if (pack.recver >= 0){
				if (SearchInPool(pack.recver, recv_node)){ // receiver online
					int ret = write(recv_node.connfd, (char*)&pack, sizeof(pack));
					if (ret > 0){
						printf("send \"%s\" | form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
					}
					else if (ret == 0)break;
					else printf("### Failed to send message(%d)\n", ret);
				}
				else { // offline
					HangUpMsg(pack.recver, pack);
				}
			}
		}
		else if (ret == 0)break;
		else printf("### Failed to receive message(%d)\n", ret);
	}
    close(node.connfd);
	EraseFromPool(node.account);
	pthread_exit(NULL);
}

SocketServer::SocketServer(){}

void* SocketServer::connect(void* args){
	//接收客户端请求
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

	while(1){
		usleep(1e4);
		Node node;
		node.connfd = accept(m_servfd, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
		if (node.connfd == -1) {
			perror("### accept() error\n"); 
			continue;
        }
		printf("### You got a connection from %s (%d)\n", inet_ntoa(clnt_addr.sin_addr), node.connfd);

		TransPack pack;
		/* waiting for account set */
		bool connected = false;
		while(true){
			usleep(1e4);
			int ret = read(node.connfd, (char*)&pack, sizeof(pack));
			if (ret > 0){
				printf("receive request \"%s\" | form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
				if (pack.recver == -1){
					node.account = pack.sender;
					connected = true;
					break;
				}
			}
			else {
				connected = false;
				break;
			}
		}
		if (!connected)continue;
		pack.sender = -1;
		int ret = write(node.connfd, (char*)&pack, sizeof(pack));
		if (ret <= 0){
			printf("### failed to send check signal. %d\n", node.connfd);
			continue;
		}
		printf("### Connection established from %d\n", node.account);

		usleep(1e6);

		/* Get offline MSG */
		std::list< std::list<TransPack> >::iterator iter;
		if (SearchInQueue(node.account, iter)){
			while(!iter->empty()){
				pack = iter->front();
				int ret = write(node.connfd, (char*)&pack, sizeof(pack));
				if (ret > 0){
					printf("send \"%s\" | form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
					iter->pop_front();
				}
				else if (ret == 0)break;
				else printf("### Failed to send message(%d)\n", ret);
			}
		}

		ret = pthread_create(&node.trans_thread, NULL, transmit, (void*)&(node));
		if (ret != 0) {
			printf("### pthread_create for transmit thread error: error_code = %d\n", ret);
			exit(-1);
		}
		pthread_detach(node.trans_thread);
		g_conn_pool.push_back(node);
	}
}

SocketServer::~SocketServer(){
	pthread_cancel(conn_thread);
	close(m_servfd);
}

bool SocketServer::init(const char* ip, const int port){
	//创建套接字
    m_servfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//将套接字和IP、端口绑定
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr(ip);  //具体的IP地址
    serv_addr.sin_port = htons(port);  //端口
    int ret = bind(m_servfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret != 0){
		printf("### failed to bind: error_code = %d\n", ret);
		return false;
	}

    //进入监听状态，等待用户发起请求
    listen(m_servfd, 20);

	printf("### start listening.\n");

	ret = pthread_create(&conn_thread, NULL, connect, NULL);
	if (ret != 0) {
		printf("### pthread_create for conn_thread error: error_code = %d\n", ret);
		return false;
	}

	while(1);

	return true;
}