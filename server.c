#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/select.h>
#include "dns.h"

#define BUF_SIZE 10240
static const int QUEUE_SIZE = 10;

// 	无服务监听的地址
static char* listenIp = NULL;
static int listenPort = 0;

void* worker(void* fd) {
	char buf[BUF_SIZE];
	memset(buf, 0, sizeof(buf));
	
	/**** 客户端，服务端协商验证方法 ***/
	if (recv((int)fd, buf, sizeof(buf), 0) < 0) {
		perror("recv");
		close((int)fd);
		return NULL;
	}
	
	// 验证和处理客户端发过来的数据
	int version =(int)buf[0];
	if (version != 5) {
		printf("not socks5 protocol\n");
		close((int)fd);
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
		close((int)fd);
		return NULL;
	}

	/***** 协商认证阶段完成 *******/

	/**** 处理客户端发过来的数据包 ****/
	memset(buf, 0, sizeof(buf));
	if (recv((int)fd, buf, sizeof(buf), 0) < 0) {
		perror("recv");
		close((int)fd);
		return NULL;
	}
	unsigned short aType = (unsigned short)buf[3];
	char *name = NULL;
	int nameSize = 0;
	// 解析客户端数据包，获取ip以及端口
	struct sockaddr_in remoteAddr;
	memset(&remoteAddr, 0, sizeof(remoteAddr));
	remoteAddr.sin_family = AF_INET;
	switch(aType) {
		case 1:
			// 地址类型为ipv4
			remoteAddr.sin_addr.s_addr = *((int*)(buf + 4));
			memcpy(&(remoteAddr.sin_port), buf + 8, 2);
			break;
		case 3: 
			nameSize = (unsigned short)buf[4];
			name = malloc(nameSize + 1);
			memset(name, 0, nameSize + 1);
			memcpy(name, buf + 5, nameSize);
			remoteAddr.sin_addr.s_addr = inet_addr(nameToIp(name));
			memcpy(&(remoteAddr.sin_port), buf + 5 + nameSize, 2);
			break;
		default:
			printf("address type is wrong\n");
			close((int)fd);
			return NULL;

	}

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
		close((int)fd);
		return NULL;
	}

	/*** 处理客户端数据结束 **/

	/*** 转发客户端数据给远程服务器并且将远程服务器的响应转发给客户端 ***/

	int remoteFd = socket(AF_INET, SOCK_STREAM, 0);
	if (remoteFd < 0) {
		perror("socket");
		close((int)fd);
		return NULL;
	}
	if (connect(remoteFd, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr)) < 0) {
		perror("connect");
		close((int)fd);
		close(remoteFd);
		return NULL;
	}

	fd_set fdSet;
	fd_set fdSetBak;

	FD_ZERO(&fdSet);
	FD_SET((int)fd, &fdSet);
	FD_SET(remoteFd, &fdSet);

	fdSetBak = fdSet;
	
	int _s = 0;

	int nfds = (int)fd > remoteFd ? (int)fd + 1 : remoteFd + 1;
	while(1) {
		if (select(nfds, &fdSet, NULL, NULL, 0) < 0) {
			perror("select");
			close((int)fd);
			close(remoteFd);
			return NULL;	
		}	
		if (FD_ISSET((int)fd, &fdSet)) {
			memset(buf, 0, sizeof(buf));
			if ((_s = recv((int)fd, buf, sizeof(buf), 0)) < 0) {
				perror("recv");
				close((int)fd);
				close(remoteFd);
				return NULL;
			}
			if (send(remoteFd, buf, _s, 0) < 0) {
				printf("%s\n", buf);
				perror("send");
				close((int)fd);
				close(remoteFd);
				return NULL;
			}
		}
		if (FD_ISSET(remoteFd, &fdSet)) {
			memset(buf, 0, sizeof(buf));
			if ((_s = recv(remoteFd, buf, sizeof(buf), 0)) < 0) {
				perror("recv");
				close((int)fd);
				close(remoteFd);
				return NULL;
			}
			if (send((int)fd, buf, _s, 0) < 0) {
				printf("%s\n", buf);
				perror("send");
				close((int)fd);
				close(remoteFd);
				return NULL;
			}
		}
		fdSet = fdSetBak;
	}

	return NULL;
}


int main(int argc,  char* argv[]) {
	if (argc < 3) {
		printf("missing parameters\n");
		return 1;
	}

	signal(SIGPIPE, SIG_IGN); // 忽略SIGPIPE信号，不然进程会被默认处理，即终止掉

	listenIp = argv[1];
	listenPort = atoi(argv[2]);
		
	int listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd < 0) {
		perror("socket");
		return 2;
	}
	struct sockaddr_in listenAddr;
	memset(&listenAddr, 0, sizeof(listenAddr));
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_addr.s_addr = inet_addr(listenIp);
	listenAddr.sin_port = htons(listenPort);

	if (bind(listenFd, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) < 0) {
		perror("bind");
		return 3;
	}

	if (listen(listenFd, QUEUE_SIZE) < 0) {
		perror("listen");
		return 4;
	}

	while (1) {
		int clientFd = accept(listenFd, NULL, NULL);	
		if (clientFd < 0) {
			perror("accept");
			return 5;
		}

		pthread_t tid;
		int ret;
		if ((ret = pthread_create(&tid, NULL, worker, (void*)clientFd)) != 0) {
			printf("pthread_create:%d", ret);
		}
	}


	return 0;
}
