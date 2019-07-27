#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

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
		int clientFd = 0;
		while (read(fds[0], &clientFd, sizeof(int))) {
			printf("client:%d\n", clientFd);
			
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















