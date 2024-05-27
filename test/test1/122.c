int test2(void) {
    int arr[10];
    int *p = arr;
    p++; // Valid: p is a pointer type and a modifiable lvalue
    return *p;
}