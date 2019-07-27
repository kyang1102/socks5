/**
 * 查找元素在数组中的位置
 * ele 要查找的元素
 * arr 数组
 * size 数组的大小
 */
int arraySearch(int ele, int arr[], int size) {
	int i = 0;
	for (i = 0; i < size; i++) {
		if (ele == arr[i]) {
			return i;
		}
	}
	return -1;
}

/**
 * 从数组中移除指定元素
 * ele 要移除的元素
 * arr 数组
 * size 数组的大小
 */
int removeElement(int ele, int arr[], int size) {
	int *tmpArr = (int*)malloc(sizeof(int) * size);
	memset(tmpArr, 0, size * sizeof(int));
	int i = 0, idx = 0;
	for (i = 0; i < size; i++) {
		if (arr[i] != ele) {
			tmpArr[idx++] = arr[i];
		}
	}
	memcpy(arr, tmpArr, size * sizeof(int));
	free(tmpArr);
	return size - (idx + 1);
}
