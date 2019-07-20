#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>

#define BUF_SIZE 10240
static const int QUEUE_SIZE = 100;

// 	无服务监听的地址
static char* listenIp = NULL;
static int listenPort = 0;

// 域名解析
int nameToIp(char* name) {
	return inet_addr("180.101.49.12");
}

void* worker(void* fd) {
	char buf[BUF_SIZE];
	memset(buf, 0, sizeof(buf));
	
	/**** 客户端，服务端协商验证方法 ***/
	if (recv((int)fd, buf, sizeof(buf), 0) < 0) {
		perror("recv");
		return NULL;
	}
	
	// 验证和处理客户端发过来的数据
	int version =(int)buf[0];
	if (version != 5) {
		printf("not socks5 protocol\n");
		return NULL;
	}
	int nmethods = (int)buf[1];
	int size = nmethods * sizeof(int);
	int *methods = malloc(size);	
	memset(methods, 0, size);
	int i = 0;
	for(i = 0; i < nmethods; i++) {
		methods[i] = (int)buf[2 + i];
	}

	// 响应客户端
	memset(buf, 0, sizeof(buf));
	buf[0] = 5;
	buf[1] = 0;
	if (send((int)fd, buf, 2, 0) < 0) {
		perror("send");
		return NULL;
	}

	/***** 协商认证阶段完成 *******/

	/**** 处理客户端发过来的数据包 ****/
	memset(buf, 0, sizeof(buf));
	if (recv((int)fd, buf, sizeof(buf), 0) < 0) {
		perror("recv");
		return NULL;
	}
	unsigned short aType = (unsigned short)buf[3];
	unsigned long remoteIp;
	unsigned short remotePort;
	unsigned short nameSize;
	char *name = NULL;
	switch(aType) {
		case 1:
			// 地址类型为ipv4
			remoteIp = *((int*)(buf + 4));
			memcpy(&remotePort, buf + 8, 2);
			break;
		case 3: 
			nameSize = (unsigned short)buf[4];
			name = malloc(nameSize + 1);
			memset(name, 0, nameSize + 1);
			memcpy(name, buf + 5, nameSize);
			remoteIp = nameToIp(name);
			memcpy(&remotePort, buf + 4 + nameSize, 2);
			break;
		default:
			printf("address type is wrong\n");
			return NULL;

	}
	// 解析客户端数据包，获取ip以及端口
	struct sockaddr_in remoteAddr;
	memset(&remoteAddr, 0, sizeof(remoteAddr));
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_addr.s_addr = remoteIp;
	remoteAddr.sin_port = remotePort;
	remoteAddr.sin_port = remotePort;

	printf("remote addr %s:%d\n", inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));

	// 响应客户端发送的数据波
	memset(buf, 0, sizeof(buf));
	buf[0] = 5;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 1;

	int ip = inet_addr(listenIp);
	memcpy(buf + 4, &ip, 4);
	memcpy(buf + 8, &listenPort, 2);

	if (send((int)fd, buf, 10, 0) < 0) {
		perror("send");
		return NULL;
	}

	/*** 处理客户端数据结束 **/

	/*** 转发客户端数据给远程服务器并且将远程服务器的响应转发给客户端 ***/

	memset(buf, 0, sizeof(buf));
	int _s = 0;
	if ((_s = recv((int)fd, buf, sizeof(buf), 0)) < 0) {
		perror("recv");
		return NULL;
	}

	int remoteFd = socket(AF_INET, SOCK_STREAM, 0);
	if (remoteFd < 0) {
		perror("socket");
		return NULL;
	}
	if (connect(remoteFd, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr)) < 0) {
		perror("connect");
		return NULL;
	}
	
	if (send(remoteFd, buf, _s, 0) < 0) {
		perror("send");
		return NULL;
	}
	
	memset(buf, 0, sizeof(buf));
	if ((_s = recv(remoteFd, buf, sizeof(buf), 0)) < 0) {
		perror("recv");
		return NULL;
	}
	
	printf("data len:%d\n", _s);
	if (send((int)fd, buf, _s, 0) < 0) {
		perror("send");
		return NULL;
	}
	
	return NULL;
}


int main(int argc,  char* argv[]) {
	if (argc < 3) {
		printf("missing parameters\n");
		return -1;
	}

	listenIp = argv[1];
	listenPort = atoi(argv[2]);
		
	int listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd < 0) {
		perror("socket");
		return -2;
	}
	struct sockaddr_in listenAddr;
	memset(&listenAddr, 0, sizeof(listenAddr));
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_addr.s_addr = inet_addr(listenIp);
	listenAddr.sin_port = htons(listenPort);

	if (bind(listenFd, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) < 0) {
		perror("bind");
		return -3;
	}

	if (listen(listenFd, QUEUE_SIZE) < 0) {
		perror("listen");
		return -4;
	}

	while (1) {
		int clientFd = accept(listenFd, NULL, NULL);	
		if (clientFd < 0) {
			perror("accept");
			return -5;
		}

		pthread_t tid;
		int ret;
		if ((ret = pthread_create(&tid, NULL, worker, (void*)clientFd)) != 0) {
			printf("pthread_create:%d");
		}
	}


	return 0;
}
