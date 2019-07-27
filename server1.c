#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "array.h"

#define SERVER_IP "0.0.0.0"
#define SERVER_PORT 8888
#define QUEUE_SIZE 100

int main() {

	int fds[2] = {0};
	if (pipe(fds) < 0) {
		perror("pipe");
		return -1;
	}
	int pid = fork();
	if (pid < 0) {
		perror("fork");
		return -1;
	} else if (pid == 0) {
		// child
		close(fds[1]);
		
		fd_set readFdSet, readFdSetBak;
		FD_ZERO(readFdSet);
		FD_SET(fds[0], &readFdSet);
		FD_COPY(&readFdSet, &readFdSetBak);
		
		/*
		int
     select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
         fd_set *restrict errorfds, struct timeval *restrict timeout);
		*/ 
		
		int nfds = fds[0] + 1;
		int clientFd = 0;

		int clientFds[200] = {0};
		int remoteFds[200] = {0};
		int cur = 0;

		char buf[BUF_SIZE] = {0};

		while (1) {
			FD_COPY(&readFdSetBak, &readFdSet);
			if (select(nfds, &readFdSet, NULL, NULL, NULL) < 0) {
				perror("select");
				return -1;
			}
			int i = 0;
			for (i = 0; i < FD_SIZE; i++) {
				if (FD_ISSET(i, &readFdSet)) {
					if (i == fds[0]) {
						if (read(fds[0], &clientFd, sizeof(clientFd)) < 0) {
							perror("read");
							return -1;
						}
						FD_SET(clientFd, &readFdSetBak);
					} else {
						if (arraySearch(i, clientFds) == -1) {
							// 客户端发来消息 建立socks5连接
						} else if (arraySearch(i, remoteFds) == -1) {
							// 	远端服务器发来消息
						} else {
							// 	客户端发来消息 未建立socks5连接			
							memset(buf, 0, sizeof(buf));
							if (recv(i, buf, sizeof(buf), 0) < 0) {
								perror("recv");
								close(i);
							
							}
							
						
										
						}
						

					
					}		
				}
			}				
			
			
		}
	} else {
		// parent
		int listenFd = socket(AF_INET, SOCK_STREAM, 0);
		if (listenFd < 0) {
			perror("socket");
			return -1;
		}	
		struct sockaddr_in listenAddr;
		memset(&listenAddr, 0, sizeof(listenAddr));

		listenAddr.sin_family = AF_INET;
		listenAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
		listenAddr.sin_port = htons(SERVER_PORT);
		if (bind(listenFd, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) < 0) {
			perror("bind");
			return -1;
		}
		if (listen(listenFd, QUEUE_SIZE) < 0) {
			perror("listen");
			return -1;
		}		

		close(fds[0]);
		int clientFd = 0;
		while (1) {
			if ((clientFd = accept(listenFd, NULL, NULL)) < 0) {
				perror("accept");
				return -1;
			}
			if (write(fds[1], &clientFd, sizeof(int)) < 0) {
				perror("write");
				return -1;
			}
		}
	}


}















