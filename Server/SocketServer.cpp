#include "SocketServer.h"

int SocketServer::m_servfd = -1;
std::list<Node> SocketServer::g_conn_pool = std::list<Node>();
std::list< std::list<TransPack> > SocketServer::g_msg_queue = std::list< std::list<TransPack> >();
DBController SocketServer::m_dbcontroller = DBController();

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
		int ret = recv(node.connfd, (char*)&pack, sizeof(pack), 0);
        if (ret > 0){
			printf("receive(id=%d) \"%s\" | form %d to %d at %s\n", pack.id, pack.msg, pack.sender, pack.recver, asctime(&pack.t));
			Node recv_node;
			if (pack.id >= 0){
				if (pack.recver == SERVER_IDX){ // apply for records
					std::list<TransPack> list;
					if (!m_dbcontroller.getRecords(pack.sender, atoi(pack.msg), pack.id, list, 10)){
						printf("### failed in getRecords\n");
					}
					recv_node = node;
					while (!list.empty()){
						int ret = send(recv_node.connfd, (char*)&(list.front()), sizeof(TransPack), 0);
						if (ret > 0){
							printf("send \"%s\" | form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
						}
						else if (ret == 0)break;
						else printf("### Failed to send message(%d)\n", ret);
						list.pop_front();
					}
					if (ret <= 0)break;
				}
				else { // transmit
					m_dbcontroller.createRecord(pack);
					if (SearchInPool(pack.recver, recv_node)){
						int ret = send(recv_node.connfd, (char*)&pack, sizeof(pack), 0);
						if (ret > 0){
							printf("send \"%s\" | form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
						}
						else if (ret == 0)break;
						else printf("### Failed to send message(%d)\n", ret);
					}
				}
			}
			else if (pack.id == D_FILE_PACK){ // TODO write url into DB
				if (SearchInPool(pack.recver, recv_node)){
					int ret = send(recv_node.connfd, (char*)&pack, sizeof(pack), 0);
					if (ret > 0){
						printf("send img pack | form %d to %d at %s\n", pack.sender, pack.recver, asctime(&pack.t));
					}
					else if (ret == 0)break;
					else printf("### Failed to send image pack(%d)\n", ret);
				}
			}
			else if (pack.id == D_FILE_END){
				if (SearchInPool(pack.recver, recv_node)){
					int ret = send(recv_node.connfd, (char*)&pack, sizeof(pack), 0);
					if (ret > 0){
						printf("send %s| form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
					}
					else if (ret == 0)break;
					else printf("### Failed to send image end(%d)\n", ret);
				}
			}
			else if (pack.id == R_APPLICANT){
				m_dbcontroller.createFriendLinkRequest(pack); // add request_id
				if (SearchInPool(pack.recver, recv_node)){ // send to target
					int ret = send(recv_node.connfd, (char*)&pack, sizeof(pack), 0);
					if (ret > 0){
						printf("send \"%s\" | form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
					}
					else if (ret == 0)break;
					else printf("### Failed to send message(%d)\n", ret);
				}
			}
			else if (pack.id == R_REPLY){
				m_dbcontroller.replyFriendLinkRequest(pack);
				int request_id, state;
				request_id = atoi(strtok(pack.msg, "/"));
				state = atoi(strtok(NULL, "/"));
				if (state == 1){ // accept
					// Get Friend List
					std::list<TransPack> list;
					m_dbcontroller.getFriends(node.account, list);
					while (!list.empty()){
						int ret = send(node.connfd, (char*)&(list.front()), sizeof(TransPack), 0);
						if (ret <= 0){
							printf("### failed to send friend list. %d\n", node.connfd);
							break;
						}
						list.pop_front();
					}
					if (SearchInPool(pack.recver, recv_node)){ // send to app_account
						m_dbcontroller.getFriends(recv_node.account, list);
						while (!list.empty()){
							int ret = send(recv_node.connfd, (char*)&(list.front()), sizeof(TransPack), 0);
							if (ret <= 0){
								printf("### failed to send friend list. %d\n", node.connfd);
								break;
							}
							list.pop_front();
						}
					}
				}
				else if (state == -1) { //reject
					if (SearchInPool(pack.recver, recv_node)){ // send to app_account
						int ret = send(recv_node.connfd, (char*)&(pack), sizeof(pack), 0);
						if (ret <= 0){
							printf("### failed to send app failed pack. %d\n", node.connfd);
							break;
						}
						list.pop_front();
					}
				}
			}
			else if (pack.id == R_UPDATEPWD){
				m_dbcontroller.updatePWD(pack);
				int ret = send(node.connfd, (char*)&(pack), sizeof(pack), 0);
				if (ret <= 0){
					printf("### failed to send pwd change result. %d\n", node.connfd);
					break;
				}
			}
			else if (pack.id == R_UPDATEALIAS){
				m_dbcontroller.updateAlias(pack);
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
		while(!connected){
			usleep(1e4);
			int ret = recv(node.connfd, (char*)&pack, sizeof(pack), 0);
			if (ret > 0){
				printf("receive request \"%s\" | form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
				if (pack.id == R_LOGIN){
					if (m_dbcontroller.login(pack) && strcmp(pack.msg, "") != 0){
						node.account = pack.recver;
						connected = true;
						break;
					}
					else{
						printf("login Failed\n");
						int ret = send(node.connfd, (char*)&pack, sizeof(pack), 0);
						if (ret <= 0)break;
					}
				}
				else if (pack.id == R_REGISTER){
					m_dbcontroller.createAccount(pack);
					node.account = pack.recver;
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
		int ret = send(node.connfd, (char*)&pack, sizeof(pack), 0);
		if (ret <= 0){
			printf("### failed to send check signal. %d\n", node.connfd);
			continue;
		}
		printf("### Connection established from %d\n", node.account);

		usleep(1e6);

		// Get Friend List
		std::list<TransPack> list;
		m_dbcontroller.getFriends(node.account, list);
		while (!list.empty()){
			int ret = send(node.connfd, (char*)&(list.front()), sizeof(TransPack), 0);
			if (ret <= 0){
				printf("### failed to send friend list. %d\n", node.connfd);
				break;
			}
			list.pop_front();
		}
		if (ret <= 0)continue;

		/* Get offline Message and Transaction */
		// Search in DB
		m_dbcontroller.getUnreadRecords(node.account, list);
		while (!list.empty()){
			int ret = send(node.connfd, (char*)&(list.front()), sizeof(TransPack), 0);
			if (ret <= 0){
				printf("### failed to send history records. %d\n", node.connfd);
				break;
			}
			m_dbcontroller.setRecordRead(list.front().id);
			list.pop_front();
		}
		if (ret <= 0)continue;

		m_dbcontroller.getFriendLinkRequests(node.account, list);
		while (!list.empty()){
			int ret = send(node.connfd, (char*)&(list.front()), sizeof(TransPack), 0);
			if (ret <= 0){
				printf("### failed to send friend link requests. %d\n", node.connfd);
				break;
			}
			list.pop_front();
		}
		if (ret <= 0)continue;
		
		/* Search in Queue
		std::list< std::list<TransPack> >::iterator iter;
		if (SearchInQueue(node.account, iter)){
			while(!iter->empty()){
				pack = iter->front();
				int ret = send(node.connfd, (char*)&pack, sizeof(pack), 0);
				if (ret > 0){
					printf("send \"%s\" | form %d to %d at %s\n", pack.msg, pack.sender, pack.recver, asctime(&pack.t));
					iter->pop_front();
				}
				else if (ret == 0)break;
				else printf("### Failed to send message(%d)\n", ret);
			}
		}*/

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