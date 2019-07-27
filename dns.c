#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

char* nameToIp(const char* name) {
	char *ip = NULL;
	
	struct hostent* hostent = gethostbyname(name);
	if (hostent == NULL) {
		return NULL;
	}
	struct in_addr in_addr;
	memset(&in_addr, 0, sizeof(in_addr));
	memcpy(&(in_addr.s_addr), hostent->h_addr, 4);
	return inet_ntoa(in_addr);
}
