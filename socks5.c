#define BUF_SIZE 1024
/**
 * socksx协议认证
 *
 * 认证成功返回1
 * 认证失败返回0
 */ 
int authenticate(int fd) {
	char buf[BUF_SIZE] = {0};
	if (recv(fd, buf, sizeof(buf), 0) < 0) {
		return 0;
	}
	int version = (int)buf[0];
	if (version != 5) {
		return 0;
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
		return 0;
    }
	return 1;
}


/**
 * 请求代理
 * fd 表示客户端文件描述符
 * addr 返回的客户端需要请求的远端地址
 * 出错返回 -1， 成功返回0
 */

int requestAgent(int fd, struct sockaddr_in* addr) {
	

	return 0;
}

