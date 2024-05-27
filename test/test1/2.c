// Example 2: File scope function prototype
int file_function(int x);

int foo(int val) {
    return file_function(val);
}

int file_function(int x) {
    return x * 2;
}