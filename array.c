#include "socks5.h"

int removeByKey(int key, socks5_req arr[], int* size) {
	if (key >= *size) {
		return -1;
	}
	int i;
	for(i = key; i < *size - 1; i++) {
		arr[i] = arr[i + 1];
	}
	(*size)--;
	return 0;
}
