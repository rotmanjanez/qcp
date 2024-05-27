int test5(void) {
    int arr[10];
    arr++; // Invalid: arr is not a modifiable lvalue, it's an array
    return 0;
}