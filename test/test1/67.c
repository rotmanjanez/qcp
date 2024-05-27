int foo(int val) {
    int arr[3] = {1, 2, 3}; // arr[i] is an lvalue and designates an object of type int
    arr[1] = val;           // using arr[1] as an lvalue
    return arr[1];          // using arr[1] as an lvalue
}