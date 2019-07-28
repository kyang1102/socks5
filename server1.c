#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <stdlib.h>
#include "array.h"

#define SERVER_IP "0.0.0.0"
#define SERVER_PORT 8888
#define QUEUE_SIZE 100
#define BUF_SIZE 500

int getMaxFd(struct fd_set fds) {
	int maxFd = -1;
	int i;
	for (i = 0; i < FD_SETSIZE; i++) {
		if (FD_ISSET(i, &fds) && maxFd < i) {
			maxFd = i;
		}
	}
	return maxFd;
}

int main() {
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

	fd_set readFdSet, readFdSetBak;
	FD_ZERO(&readFdSet);
	FD_SET(listenFd, &readFdSet);
	FD_COPY(&readFdSet, &readFdSetBak);
	
	int nfds = getMaxFd(readFdSetBak) + 1;
	int clientFd = 0;

	socks5_req srArr[500];
	memset(srArr, 0, sizeof(socks5_req) * 500);
	int cur = 0;

	char buf[BUF_SIZE] = {0};
	int _s;

	struct sockaddr_in remoteAddr;
	int remoteFd;

	int targetFd;
	while (1) {
		FD_COPY(&readFdSetBak, &readFdSet);
		printf("here1,nfds:%d\n", nfds);
		if (select(nfds, &readFdSet, NULL, NULL, NULL) < 0) {
			perror("select");
			return -1;
		}
		printf("here\n");
		int i = 0;
		for (i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &readFdSet)) {
				if (i == listenFd) {
					if ((clientFd = accept(listenFd, NULL, NULL)) < 0) {
						perror("accept");
						return -1;
					}
					printf("recive a client:%d\n", clientFd);
					FD_SET(clientFd, &readFdSetBak);
					nfds = getMaxFd(readFdSetBak) + 1;
					socks5_req sr;
					sr.status = STATUS_NEG_AUTH;
					sr.client_fd = clientFd;
					srArr[cur++] = sr; 
				} else {
					int j;
					for (j = 0; j < cur; j++) {
						if (srArr[j].client_fd == i || (srArr[j].status == STATUS_ESTABLISH && srArr[j].remote_fd == i)) {
							break;		
						}
					}
					printf("status:%d\n", srArr[j].status);
					switch(srArr[j].status) {
						case STATUS_NEG_AUTH:
							if (negotiatedAuth(i) < 0) {
								FD_CLR(i, &readFdSetBak);
								nfds = getMaxFd(readFdSetBak) + 1;
								removeByKey(j, srArr, &cur);	
								close(i);
							} else {
								srArr[j].status = STATUS_REQ_AGT;
							}
							break;
						case STATUS_REQ_AGT:
							memset(&remoteAddr, 0, sizeof(remoteAddr));
							if (requestAgent(i, &remoteAddr, listenAddr) < 0) {
								FD_CLR(i, &readFdSetBak);
								nfds = getMaxFd(readFdSetBak) + 1;
								removeByKey(j, srArr, &cur);	
								close(i);
							} else {
								srArr[j].status = STATUS_ESTABLISH;
								remoteFd = socket(AF_INET, SOCK_STREAM, 0);
								if (remoteFd < 0) {
									FD_CLR(i, &readFdSetBak);
									nfds = getMaxFd(readFdSetBak) + 1;
									removeByKey(j, srArr, &cur);
									close(i);
								}	
								if (connect(remoteFd, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr)) < 0) {
									FD_CLR(i, &readFdSetBak);
									nfds = getMaxFd(readFdSetBak) + 1;
									removeByKey(j, srArr, &cur);
									close(i);
								}
								srArr[j].remote_fd = remoteFd;
								FD_SET(remoteFd, &readFdSetBak);
								nfds = getMaxFd(readFdSetBak) + 1;
							}
							break;
						case STATUS_ESTABLISH:
							memset(buf, 0, sizeof(buf));
							if ((_s = recv(i, buf, sizeof(buf), 0)) < 0) {
								FD_CLR(srArr[j].client_fd, &readFdSetBak);
								FD_CLR(srArr[j].remote_fd, &readFdSetBak);
								nfds = getMaxFd(readFdSetBak) + 1;
								removeByKey(j, srArr, &cur);
								close(srArr[j].client_fd);
								close(srArr[j].remote_fd);
							}	
							if (i == srArr[j].client_fd) {
								targetFd = srArr[j].remote_fd;		
							} else {
								targetFd = srArr[j].client_fd;
							}

							if (send(targetFd, buf, _s, 0) < 0) {
								FD_CLR(srArr[j].client_fd, &readFdSetBak);
								FD_CLR(srArr[j].remote_fd, &readFdSetBak);
								nfds = getMaxFd(readFdSetBak) + 1;
								removeByKey(j, srArr, &cur);
								close(srArr[j].client_fd);
								close(srArr[j].remote_fd);
							}
							printf("%d say to %d, size:%d, mess:%s\n", i, targetFd, _s,  buf);
							break;
					}
				
				}		
			}
		}				
	}
	return 0;
}

