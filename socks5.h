#include <arpa/inet.h>
/**
 * socks5请求结构体
 * status表示请求所处的阶段
 * client_fd 表示客户端的描述符
 * remote_fd 表示目标端的描述符
 */ 
typedef struct {
	int status;
	int client_fd;
	int remote_fd;
} socks5_req;

#define STATUS_NEG_AUTH 0
#define STATUS_REQ_AGT 1
#define STATUS_ESTABLISH 2

/**
 * 协商认证
 * fd 客户端描述符
 * 0表示成功，-1表示失败
 */
int negotiatedAuth(int fd);

/**
 * 请求代理
 * fd 表示客户端文件描述符
 * addr 返回的客户端需要请求的远端地址
 * listenAddr 表示服务器监听的地址
 * 出错返回 -1， 成功返回0
 */

int requestAgent(int fd, struct sockaddr_in* addr, struct sockaddr_in listenAddr);
