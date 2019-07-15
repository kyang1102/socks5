#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>

static char* ip = "127.0.0.1";
static int port = 2019;

int main() {

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);
    
    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        return -2;
    }

    if (listen(serverFd, 100) < 0) {
        perror("listen");
        return -3;
    }
    
    printf("listen...\n");

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr); 
    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientFd < 0) {
        perror("accept");
        return -4;
    }

    printf("%s:%d connnet successfully\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    char buf[20000] = {0};
    if (recv(clientFd, buf, sizeof(buf), 0) <= 0) {
        perror("recv");
        return -5;
    }

    int version = buf[0];
    if (version != 5) {
        return -6;
    }

    int nmethods = buf[1];
    char* methods = malloc(nmethods);
    memcpy(methods, buf + 2, nmethods);
    
    memset(buf, 0, sizeof(buf));
    buf[0] = 5;
    buf[1] = 0;
    if (send(clientFd, buf, 2, 0) < 0) {
        perror("send");
        return -7;
    }

    memset(buf, 0, sizeof(buf));
    if (recv(clientFd, buf, sizeof(buf), 0) <= 0) {
        perror("recv");
        return -8;
    }
    int aType = buf[3];
    if (aType != 1) {
        printf("not ipv4");
        return -8;
    }

	// 与目标服务器进行连接，进行通信
	struct sockaddr_in remoteAddr;
	memset(&remoteAddr, 0, sizeof(remoteAddr));
	remoteAddr.sin_family = AF_INET;
	memcpy(&(remoteAddr.sin_addr), buf + 4, 4);
	memcpy(&(remoteAddr.sin_port), buf + 8, 2);
    printf("remote addr %s:%d\n", inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));

	int sockFd;
	if ((sockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -9;
	}

	if (connect(sockFd, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr)) < 0) {
		perror("connect");
		return -10;
	}

	memset(buf, 0, sizeof(buf));
	buf[0] = 0x5;
	buf[1] = 0x0;
	buf[2] = 0x0;
	buf[3] = 0x1;
	in_addr_t netIp = inet_addr(ip);
	int netPort = htons(port);
	memcpy(buf + 4, &netIp, 4);
	memcpy(buf + 8, &netPort, 2);

	if (send(clientFd, buf, 10, 0) < 0) {
		perror("send");
		return -11;	
	}
	
	memset(buf, 0, sizeof(buf));
	int _s = 0;
	if ((_s = recv(clientFd, buf, sizeof(buf), 0)) <= 0) {
		perror("recv");
		return -12;
	}

	if (send(sockFd, buf, _s, 0) < 0) {
		perror("send");
		return -13;
	}

	memset(buf, 0, sizeof(buf));
	if ((_s = recv(sockFd, buf, sizeof(buf), 0)) <= 0) {
		perror("recv");
		return -14;
	}

	if (send(clientFd, buf, _s, 0) < 0) {
		perror("send");
		return -15;
	}


    return 0;
}
