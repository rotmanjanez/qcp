int foo(int val) {
    int arr[3] = {1, 2, 3};
    arr = &val;          // Error: assignment to expression with array type
    return arr[0];
}