#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "array.h"

#define SERVER_IP "0.0.0.0"
#define SERVER_PORT 8889
#define QUEUE_SIZE 100
#define BUF_SIZE 500

int getMaxFd(fd_set fds) {
	int maxFd = -1;
	int i;
	for (i = 0; i < FD_SETSIZE; i++) {
		if (FD_ISSET(i, &fds) && maxFd < i) {
			maxFd = i;
		}
	}
	return maxFd;
}

int timeoutConnect(struct sockaddr_in *addr, int sec) {
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = 0;

	int remoteFd = socket(AF_INET, SOCK_STREAM, 0);
	if (remoteFd < 0) {
		perror("socket");
		return -1;
	}
	fcntl(remoteFd, F_SETFL, O_NONBLOCK);
	int ret = connect(remoteFd, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
   	if (ret < 0) {
		if (errno != EINPROGRESS) {
			perror("connect");
			close(remoteFd);
			return -1;
		}
	} else if (ret == 0) {
		return remoteFd;
	}

	fd_set rset, wset;
	FD_ZERO(&rset);
	FD_SET(remoteFd, &rset);
	wset = rset;

	ret = select(remoteFd + 1, &rset, &wset, NULL, &tv);
    if (ret <= 0) {
        close(remoteFd);
        return -1;
    }
	int error = 0;
	socklen_t len;
	if (FD_ISSET(remoteFd, &rset) || FD_ISSET(remoteFd, &wset)) {
        len = sizeof(error);
        getsockopt(remoteFd, SOL_SOCKET, SO_ERROR, &error, &len);
    }

    if (error) {
        close(remoteFd);
        return -1;
    }
    return remoteFd;	
}

int main() {
	signal(SIGPIPE, SIG_IGN);
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
	//FD_COPY(&readFdSet, &readFdSetBak);
    readFdSetBak = readFdSet;
	
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
		//FD_COPY(&readFdSetBak, &readFdSet);
		readFdSet = readFdSetBak;
		if (select(nfds, &readFdSet, NULL, NULL, NULL) < 0) {
			perror("select");
			return -1;
		}
		int i = 0;
		for (i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &readFdSet)) {
				if (i == listenFd) {
					if ((clientFd = accept(listenFd, NULL, NULL)) < 0) {
						perror("accept");
						return -1;
					}
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
								printf("remote %s:%d\n", inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));
								srArr[j].status = STATUS_ESTABLISH;
								remoteFd = timeoutConnect(&remoteAddr, 1);
								if (remoteFd < 0) {
									FD_CLR(i, &readFdSetBak);
									nfds = getMaxFd(readFdSetBak) + 1;
									removeByKey(j, srArr, &cur);
									close(i);
									printf("timeout connect turn off %d\n", i);
								} else {
									int opts = fcntl(remoteFd, F_GETFL);
									opts = opts & (~O_NONBLOCK);
									fcntl(remoteFd, F_SETFL, opts);
									srArr[j].remote_fd = remoteFd;
									FD_SET(remoteFd, &readFdSetBak);
									nfds = getMaxFd(readFdSetBak) + 1;
								}
							}
							break;
						case STATUS_ESTABLISH:
							memset(buf, 0, sizeof(buf));
							if ((_s = recv(i, buf, sizeof(buf), 0)) <= 0) {
								perror("recv1");
								printf("fd:%d, %d, %d\n", i, srArr[j].client_fd, srArr[j].remote_fd);
								FD_CLR(srArr[j].client_fd, &readFdSetBak);
								FD_CLR(srArr[j].remote_fd, &readFdSetBak);
								nfds = getMaxFd(readFdSetBak) + 1;
								close(srArr[j].client_fd);
								close(srArr[j].remote_fd);
								printf("recv1 turn off %d, %d\n", srArr[j].client_fd, srArr[j].remote_fd);
								removeByKey(j, srArr, &cur);
							}	
							if (_s > 0) {
								if (i == srArr[j].client_fd) {
									targetFd = srArr[j].remote_fd;		
								} else {
									targetFd = srArr[j].client_fd;
								}

								if (send(targetFd, buf, _s, 0) <= 0) {
									perror("send");
									FD_CLR(srArr[j].client_fd, &readFdSetBak);
									FD_CLR(srArr[j].remote_fd, &readFdSetBak);
									nfds = getMaxFd(readFdSetBak) + 1;
									removeByKey(j, srArr, &cur);
									close(srArr[j].client_fd);
									close(srArr[j].remote_fd);
									printf("send turn off %d, %d\n", srArr[j].client_fd, srArr[j].remote_fd);
								}
							}
							break;
					}
				
				}		
			}
		}				
	}
	return 0;
}

