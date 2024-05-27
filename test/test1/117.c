int test(void) {
    int arr[10];
    int *ptr = arr;
   	int value = ptr[ptr]; // Invalid: ptr is not an integer type
    return 0;
}