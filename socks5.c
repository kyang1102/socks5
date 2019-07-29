#include "dns.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define BUF_SIZE 1024

int negotiatedAuth(int fd) {
	char buf[BUF_SIZE] = {0};
	if (recv(fd, buf, sizeof(buf), 0) < 0) {
		perror("recv");
		return -1;
	}
	int version = (int)buf[0];
	if (version != 5) {
		return -1;
	}
	int nmethods = (int)buf[1];
	int *methods = (int*)malloc(nmethods * sizeof(int));
	memset(methods, 0, nmethods * sizeof(int));
	int i = 0;
    for(i = 0; i < nmethods; i++) {
        methods[i] = (int)buf[2 + i]; 
    }
	memset(buf, 0, sizeof(buf));
    buf[0] = 5;
    buf[1] = 0;
    if (send((int)fd, buf, 2, 0) < 0) {
		perror("send");
		return -1;
    }
	return 0;
}

int requestAgent(int fd, struct sockaddr_in* addr, struct sockaddr_in listenAddr) {
	char buf[BUF_SIZE] = {0};
	if (recv(fd, buf, sizeof(buf), 0) < 0) {
		perror("recv");
		return -1;
	}	
	int aType = (int)buf[3];
	char* name = NULL;
	int len;

	addr->sin_family = AF_INET;
	switch(aType) {
		case 1:
			// 地址类型为ipv4
			addr->sin_addr.s_addr = *((int*)(buf + 4));
			memcpy(&(addr->sin_port), buf + 8, 2);
			break;
		case 3:
			len = (int)buf[4];
			name = (char*)malloc(len + 1);
			memset(name, 0, len + 1);
			memcpy(name, buf + 5, len);
			printf("%s\n", name);
			addr->sin_addr.s_addr = inet_addr(nameToIp(name));
			memcpy(&(addr->sin_port), buf + 5 + len, 2);
			break;
		default:
			return -1;

	}

	memset(buf, 0, sizeof(buf));
	buf[0] = 5;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 1;
	memcpy(buf + 4, &(listenAddr.sin_addr.s_addr), 4);
	memcpy(buf + 8, &(listenAddr.sin_port), 2);

	if (send(fd, buf, 10, 0) < 0) {
		perror("send");
		return -1;
	}
	return 0;
}
