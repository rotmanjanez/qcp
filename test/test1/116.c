int test(void) {
    int arr[10];
    int index = 5;
    int value = arr[index]; // Valid: arr is "pointer to complete object type", index is integer type
    return value;
}